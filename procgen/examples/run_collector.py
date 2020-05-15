#!/usr/bin/env python

from procgen.interactive import ProcgenInteractive
from procgen import ProcgenEnv

import numpy as np

from procgen.recorder import SingleRecorder, VecRecorder

import argparse


def run_collector_ia(vision, sync, level_seed, record_dir, record_prefix, options):

    # to start recording call the script with "--record-dir <your recdir>"
    recorder = None
    if record_dir is not None:
        recorder = SingleRecorder(record_dir, prefix=f"{record_prefix}_{level_seed}")
        recorder.record_obs_as("state_ship","state_ship")
        recorder.record_obs_as("state_goals","state_goals")
        recorder.record_obs_as("state_resources","state_resources")
        recorder.record_obs_as("state_obstacles","state_obstacles")
        recorder.record_obs_as("rgb","rgb")


    kwargs = {
        "start_level": level_seed,
        "num_levels": 1,
        "additional_obs_spaces": [
            ProcgenEnv.C_Space("state_ship", False, (9,), float, (-1e6,1e6)),
            ProcgenEnv.C_Space("state_goals", False, ((options["num_goals_green"]+options["num_goals_red"])*4,), float, (-1e6,1e6)),
            ProcgenEnv.C_Space("state_resources", False, ((options["num_resources_green"]+options["num_resources_red"])*4,), float, (-1e6,1e6)),
            ProcgenEnv.C_Space("state_obstacles", False, (options["num_obstacles"]*3,), float, (-1e6,1e6))
        ],
        "options": options
        }

    ia = ProcgenInteractive(vision, sync, env_name="collector", **kwargs)
    ia.run(record_dir=record_dir, recorder=recorder)


def run_collector_env_vector(num_trials, parallel_envs, level_seed, do_render, record_dir, record_prefix, options):

    for seed in level_seed:
        recorder = None
        if record_dir is not None:
            recorder = VecRecorder(parallel_envs, record_dir, prefix=f"{record_prefix}_{seed}")
            recorder.record_obs_as("state_ship","state_ship")
            recorder.record_obs_as("state_goals","state_goals")
            recorder.record_obs_as("state_resources","state_resources")
            recorder.record_obs_as("state_obstacles","state_obstacles")
            recorder.record_obs_as("rgb","rgb")
            recorder.new_recording([True for _ in range(parallel_envs)])

        kwargs = {
            "start_level": seed,
            "num_levels": 1,
            "additional_obs_spaces": [
                ProcgenEnv.C_Space("state_ship", False, (9,), float, (-1e6,1e6)),
                ProcgenEnv.C_Space("state_goals", False, ((options["num_goals_green"]+options["num_goals_red"])*4,), float, (-1e6,1e6)),
                ProcgenEnv.C_Space("state_resources", False, ((options["num_resources_green"]+options["num_resources_red"])*4,), float, (-1e6,1e6)),
                ProcgenEnv.C_Space("state_obstacles", False, (options["num_obstacles"]*3,), float, (-1e6,1e6))
            ],
            'max_episodes_per_game': num_trials,
            "options": options
            }

        env = ProcgenEnv(num_envs=parallel_envs, env_name="collector", **kwargs)

        policy = RandomPolicy(np.arange(9))

        obs = env.reset()

        info = None

        all_done = [False]*parallel_envs

        while not all(all_done):

            if do_render:
                env.render()

            if recorder is not None:
                renders = env.get_images()

            action = np.array([policy(state) for state in obs['state_ship']])

            next_obs, rew, done, next_info = env.step(action)
            all_done = env.all_episodes_done()

            if recorder is not None:
                recorder.new_entry(render=renders, obs=obs, rew=rew, done=done, info=info, action=action)
                recorder.close(done)
                recorder.new_recording(done & ~all_done)

            obs = next_obs
            info = next_info

        env.close()



class RandomPolicy():

    def __init__(self, possible_actions):
        self.possible_actions = possible_actions

    def __call__(self, obs):
        return np.random.choice(self.possible_actions)

mode_options = {
    "goto" : {
        "world_dim": int(16),
        "init_locator_type": int(1),
        "num_goals_green": int(1),
        "num_goals_red": int(0),
        "num_resources_green": int(0),
        "num_resources_red": int(0),
        "num_fuel": int(0),
        "num_obstacles": int(0),
        "goal_max": 20.0,
        "goal_init": 0.0,
        "agent_max_fuel": 100.0,
        "agent_init_fuel": 100.0,
        "agent_max_resources": 100.0,
        "agent_init_resources_green": 20.0,
        "agent_init_resources_red": 0.0,
    },
    "gotogreen": {
        "world_dim": int(16),
        "init_locator_type": int(2),
        "num_goals_green": int(1),
        "num_goals_red": int(1),
        "num_resources_green": int(0),
        "num_resources_red": int(0),
        "num_fuel": int(0),
        "num_obstacles": int(0),
        "goal_max": 20.0,
        "goal_init": 0.0,
        "agent_max_fuel": 100.0,
        "agent_init_fuel": 100.0,
        "agent_max_resources": 100.0,
        "agent_init_resources_green": 20.0,
        "agent_init_resources_red": 20.0,
    },
    "avoidred": {
        "world_dim": int(16),
        "init_locator_type": int(3),
        "num_goals_green": int(1),
        "num_goals_red": int(1),
        "num_resources_green": int(0),
        "num_resources_red": int(0),
        "num_fuel": int(0),
        "num_obstacles": int(0),
        "goal_max": 20.0,
        "goal_init": 0.0,
        "agent_max_fuel": 100.0,
        "agent_init_fuel": 100.0,
        "agent_max_resources": 100.0,
        "agent_init_resources_green": 20.0,
        "agent_init_resources_red": 10.0,
    },
    "collectgreen": {
        "world_dim": int(16),
        "init_locator_type": int(2),
        "num_goals_green": int(1),
        "num_goals_red": int(1),
        "num_resources_green": int(1),
        "num_resources_red": int(1),
        "num_fuel": int(0),
        "num_obstacles": int(0),
        "goal_max": 10.0,
        "goal_init": 0.0,
        "agent_max_fuel": 100.0,
        "agent_init_fuel": 100.0,
        "agent_max_resources": 100.0,
        "agent_init_resources_green": 0.0,
        "agent_init_resources_red": 0.0,
    },
    "default": {
        "world_dim": int(16),
        "init_locator_type": int(2),
        "num_goals_green": int(1),
        "num_goals_red": int(1),
        "num_resources_green": int(2),
        "num_resources_red": int(2),
        "num_fuel": int(2),
        "num_obstacles": int(4),
        "goal_max": 30.0,
        "goal_init": 0.0,
        "agent_max_fuel": 100.0,
        "agent_init_fuel": 100.0,
        "agent_max_resources": 100.0,
        "agent_init_resources_green": 0.0,
        "agent_init_resources_red": 0.0,
    }
}

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--vision", choices=["agent", "human"], default="human")
    parser.add_argument('--do-sync', dest='do_sync', action='store_true', help="sync steps with player input")
    parser.set_defaults(do_sync=False)
    parser.add_argument('--no-render', dest='do_render', action='store_false', help="games won't be rendered")
    parser.set_defaults(do_render=True)
    parser.add_argument("--num-envs", type=int, default=1, help="environments per vector. run in parallel. total trials = --num-envs * --num-trials")
    parser.add_argument("--num-trials", type=int, default=1, help="number of trials. total trials = --num-envs * --num-trials ")
    parser.add_argument("--record-dir", help="directory to record data to")
    parser.add_argument("--record-prefix", default="rec", help="prefix for recordings")
    parser.add_argument('--mode', help="what mode are you playing?", choices=mode_options.keys(), default='default')
    parser.add_argument('--policy', help="policy for the agent?", choices=['random','human'], default='random')
    parser.add_argument('--level-seed', nargs='+', type=int, help='list of level seeds', default=[11])
    args = parser.parse_args()


    options = mode_options[args.mode]

    record_prefix = f"{args.record_prefix}_{args.mode}"

    if args.policy == 'human':
        run_collector_ia(args.vision, args.do_sync, args.level_seed[0], args.record_dir, record_prefix, options)
    elif args.policy == 'random':
        run_collector_env_vector(args.num_trials, args.num_envs, args.level_seed, args.do_render, args.record_dir, record_prefix, options)
    else:
        print("Unknown policy")
        exit(-1)






if __name__ == "__main__":
    main()
