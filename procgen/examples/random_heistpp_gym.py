#!/usr/bin/env python
"""
Example random agent script to demonstrate that procgen works
"""

import gym
from procgen import ProcgenEnv
import argparse
import numpy as np

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vision", choices=["agent", "human"], default="human")
    parser.add_argument("--distribution-mode", default="hard", help="which distribution mode to use for the level generation")
    parser.add_argument("--level-seed", type=int, help="select an individual level to use")
    parser.add_argument("--use-generated-assets", help="using autogenerated assets", choices=["yes","no"], default="no")
    args = parser.parse_args()

    kwargs = {"distribution_mode": args.distribution_mode}
    kwargs["use_generated_assets"] = True if (args.use_generated_assets == "yes") else False
    if args.level_seed is not None:
        kwargs["start_level"] = args.level_seed
        kwargs["num_levels"] = 1

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

    env = gym.make('procgen:procgen-heistpp-v0',**kwargs)

    obs = env.reset()
    step = 0
    while True:
        env.render()
        obs, rew, done, info = env.step(np.array([env.action_space.sample()]))
        print(f"step {step} reward {rew} done {done}")
        print(info['state'])
        print(env.P)
        step += 1
        if done:
            break

    env.close()


if __name__ == "__main__":
    main()
