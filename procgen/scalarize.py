import gym
import numpy as np

class Scalarize:
    """
    Convert a VecEnv into an Env, this is basically the opposite of DummyVecEnv

    RUDIS VERSION
    """

    def __init__(self, venv) -> None:
        assert venv.num_envs == 1
        self._venv = venv
        self.num_envs = self._venv.num_envs
        self.observation_space = self._venv.observation_space
        self.action_space = self._venv.action_space
        self.metadata = self._venv.metadata

        self.spec = self._venv.spec if hasattr(self._venv, "spec") else None
        self.reward_range = self._venv.reward_range if hasattr(self._venv, "reward_range") else None

    def _process_obs(self, obs):
        return {k:v[0,...] for k, v in obs.items()} if isinstance(obs, dict) else obs[0]

    def reset(self):
        obs = self._venv.reset()
        return self._process_obs(obs)

    def step(self, action):

        if isinstance(self.action_space, gym.spaces.Discrete):
            action = np.array([action], dtype=self._venv.action_space.dtype)
        else:
            action = np.expand_dims(action, axis=0)

        obs, rews, dones, infos = self._venv.step(action)
        return self._process_obs(obs), rews[0], dones[0], infos[0]

    def render(self, mode=None):
        if mode is None:
            mode = "human"

        if mode == "human":
            return self._venv.render(mode=mode)
        else:
            assert mode == "rgb_array"
            return self.get_images()

    def get_images(self):
        return self._venv.get_images()[0]

    def close(self):
        return self._venv.close()

    def seed(self, seed=None):
        return self._venv.seed(seed)

    @property
    def unwrapped(self):
        return self

    def __repr__(self):
        return f"<Scalarize venv={self._venv}>"
