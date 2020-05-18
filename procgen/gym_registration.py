from gym.envs.registration import register
from gym import ObservationWrapper
from .env import ENV_NAMES, ProcgenEnv
from .scalarize import Scalarize

import numpy as np

class RemoveDictObs(ObservationWrapper):
    def __init__(self, env, key):
        self.key = key
        super().__init__(env=env)
        # for k,v in env.observation_space.spaces.items():
        #     print(k,v)
        self.observation_space = env.observation_space.spaces[self.key]

    def observation(self, obs):

        # recorder.record_obs_as("state_ship","state_ship")
        # recorder.record_obs_as("state_goals","state_goals")
        # recorder.record_obs_as("state_resources","state_resources")
        # recorder.record_obs_as("state_obstacles","state_obstacles")

        return np.concatenate([obs['state_ship'].flatten(),obs['state_goals'].flatten()]);
        # return obs[self.key]



def make_env(**kwargs):

    obs_key = kwargs.pop("obs_key","rgb")

    venv = ProcgenEnv(num_envs=1, num_threads=0, **kwargs)
    env = Scalarize(venv)

    env = RemoveDictObs(env, key=obs_key)
    return env


def register_environments():
    for env_name in ENV_NAMES:
        register(
            id=f'procgen-{env_name}-v0',
            entry_point='procgen.gym_registration:make_env',
            kwargs={"env_name": env_name},
        )
