#!/usr/bin/env python
import argparse

from procgen.interactive import ProcgenInteractive
from procgen import ProcgenEnv

import matplotlib.pyplot as plt
from matplotlib import colors

import numpy as np

from procgen.recorder import SingleRecorder

class HeistppStatePlotter():

    def __init__(self, world_dim, plot_interval):
        self.world_dim = world_dim
        self.plot_interval = plot_interval

        self.fig, self.axs = plt.subplots(2,1);
        plt.ion()
        plt.show()
        self.reward = []
        self.episode_return = []

        self.cmap = colors.ListedColormap([(1.0,1.0,1.0),(0.0,1.0,0.0),(0.0,0.8,0.0),(0.0,0.6,0.0),(0.0,0.0,1.0),(0.0,0.0,0.8),(0.0,0.0,0.6),(0.0,1.0,1.0),(0.8,0.0,0.0),(1.0,0.0,0.0),(0.2,0.2,0.2)])
        bounds=[0, 10.5, 11.5, 12.5, 13.5, 21.5, 22.5, 23.5, 30.5, 41.5, 42.5, 50]
        self.norm = colors.BoundaryNorm(bounds, self.cmap.N)

    def __call__(self, obs, rew, done, info, episode_steps, episode_return):

        if not episode_steps % self.plot_interval:
            self.axs[0].clear()
            img = self.axs[0].imshow(info['state'][7:].reshape(self.world_dim,self.world_dim), cmap=self.cmap, norm=self.norm)

            agent_x, agent_y = (info['state'][0] % self.world_dim), (info['state'][0] // self.world_dim);
            self.axs[0].plot(agent_x,agent_y,'ko')
            self.axs[0].invert_yaxis()


            self.axs[1].clear()
            self.reward.append(rew)
            self.episode_return.append(episode_return)
            self.axs[1].plot(self.reward)
            self.axs[1].plot(self.episode_return)

            plt.draw()
            self.fig.canvas.draw_idle()
            self.fig.canvas.start_event_loop(0.1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vision", choices=["agent", "human"], default="human")
    parser.add_argument("--record-dir", help="directory to record movies to")
    parser.add_argument("--distribution-mode", default="hard", help="which distribution mode to use for the level generation")
    parser.add_argument("--level-seed", type=int, help="select an individual level to use")
    parser.add_argument("--use-generated-assets", help="using autogenerated assets", choices=["yes","no"], default="no")
    args = parser.parse_args()

    kwargs = {"distribution_mode": args.distribution_mode}
    kwargs["use_generated_assets"] = True if (args.use_generated_assets == "yes") else False

    if args.level_seed is None:
        args.level_seed = 526

    if args.level_seed is not None:
        kwargs["start_level"] = args.level_seed
        kwargs["num_levels"] = 1

    world_dim = int(5)

    kwargs["options"] = {
        'world_dim':world_dim,
        'wall_chance':0.5,
        'fire_chance':0.0,
        'water_chance':0.0,
        'num_keys':int(0),
        'num_doors':int(0),
        'with_grid_steps':True,
        'completion_bonus':10.0,
        'fire_bonus':-5.0,
        'water_bonus':-2.0,
        'action_bonus':-1.0,
        'agent_cell':int(-1),
        'diamond_cell':int(-1),
        }





    kwargs["additional_info_spaces"] = [ProcgenEnv.C_Space("state", False, (7+world_dim*world_dim,), bytes, (0,255))]
    # SO FAR FOR HEISTPP ONLY!
    # state[0] is the current cell_index of the agent.
    # Compute coordinates: x,y = (cell_index % world_dim), (cell_index // world_dim)
    #
    # state[1:4] indicates if the agent has collected key 1,2,3 respectively.
    # 0: not collected
    # 1: collected
    #
    # state[5:7] indicates if the agent has opened door 1,2,3 respectively.
    # 0: not opened
    # 1: opened
    #
    # state[7:world_dim*world_dim+7] is the current world_map without the agent, collected keys and opened doors.
    # check 'asset_to_state' map in 'heistpp.cpp' for the definition of each state value.


    ia = ProcgenInteractive(args.vision, True, env_name="heistpp", **kwargs)
    ia.skip_info_out("state")

    # Set to True to see a "step-callback" in action
    if False:
        step_cb = HeistppStatePlotter(world_dim, 10)
        ia.add_step_callback(step_cb)


    if args.record_dir is not None:
        recorder = SingleRecorder(args.record_dir, prefix="ia")
        recorder.record_info_as("state","info_state")
        recorder.record_obs_as("rgb","obs_rgb")
    else:
        recorder = None

    ia.run(record_dir=args.record_dir, recorder=recorder)


if __name__ == "__main__":
    main()