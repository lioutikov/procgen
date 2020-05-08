#!/usr/bin/env python
"""
Example random agent script to demonstrate that procgen works
"""

from procgen import ProcgenEnv
import argparse
import numpy as np

from procgen.recorder import VecRecorder

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
    if args.level_seed is not None:
        kwargs["start_level"] = args.level_seed
        level_seed = args.level_seed
        kwargs["num_levels"] = 1
    else:
        level_seed = ''


    world_dim = int(10)
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



    kwargs["options"] = {
        'world_dim':world_dim,
        'wall_chance':0.5,
        'fire_chance':0.3,
        'water_chance':0.2,
        'num_keys':int(2),
        'num_doors':int(1),
        'with_grid_steps':True,
        'completion_bonus':10.0,
        'fire_bonus':-5.0,
        'water_bonus':-2.0,
        'action_bonus':-1.0,
        }

    num_envs = 10
    env = ProcgenEnv(num_envs=num_envs, env_name="heistpp", **kwargs)


    # to start recording call the script with "--record-dir <your recdir>"
    # we have a vector of environments and need a recorder for each one
    recorder = None
    if args.record_dir is not None:
        recorder = VecRecorder(args.record_dir, prefix=f"rand_")
        recorder.record_info_as("state","info_state")
        recorder.record_obs_as("rgb","obs_rgb")
        recorder.new_recording(np.arange(num_envs))

    obs = env.reset()
    step = 0

    while True:
        env.render()

        action = np.array([env.action_space.sample() for _ in range(num_envs)])
        obs, rew, done, info = env.step(action)
        all_episodes_done = env.all_episodes_done()

        if recorder is not None:
            renders = env.render(mode="rgb_array")
            recorder.new_entry(render=renders, obs=obs, rew=rew, done=done, info=info, action=action)
            recorder.new_recording(done & ~all_episodes_done)


        step += 1
        if all(all_episodes_done):
            break

    env.close()


if __name__ == "__main__":
    main()
