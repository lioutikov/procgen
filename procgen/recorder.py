"""
Manage the recording of render, obs, reward, done and info data for each step.


arguments:

record_dir: str # path to recordings
prefix: [optional] str # prefix for recordings produced by this instance
record_image: [optional] bool # record image? if True an mp4 of the rendered images will be created. default True
record_rew: [optional] bool # record the reward for each step? default True
record_done: [optional] bool # record done indicator for each step ? default True
continue_counter: [optional] bool # if True the new files will start from the last number+1 of existing recording in record_dir with the same prefix. default True
"""

import numpy as np
import os
import glob
import imageio

class Recorder():

    def __init__(self, record_dir, prefix = None, record_image=True, record_rew=True, record_done=True, continue_counter=True):
        self._movie_writer = None
        self._data = {}
        self._info_map = {}
        self._obs_map = {}
        self._record_image = record_image
        self._record_rew = record_rew
        self._record_done = record_done

        os.makedirs(record_dir, exist_ok=True)
        self._path_base = os.path.join(record_dir, "" if prefix is None else f"{prefix}_")


        counter = 0
        if continue_counter:
            prev_recordings = sorted(glob.glob(f"{self._path_base}[0-9][0-9][0-9].mp4") + glob.glob(f"{self._path_base}[0-9][0-9][0-9].npz"))

            if len(prev_recordings) > 0:
                counter = int(prev_recordings[-1][-7:-4])

        self._counter = counter;

        if self._record_rew:
            self._data["reward"] = []
        if self._record_done:
            self._data["done"] = []

    def record_info_as(self, name_info, name_data, transform=None):
        if name_data in self._data:
            raise KeyError(f"name already registered: {name_data}")
        self._data[name_data] = []
        self._info_map[name_data] = (name_info, transform if transform is not None else lambda _:_)

    def record_obs_as(self, name_obs, name_data, transform=None):
        if name_data in self._data:
            raise KeyError(f"name already registered: {name_data}")
        self._data[name_data] = []
        self._obs_map[name_data] = (name_obs, transform if transform is not None else lambda _:_)

    def new_recording(self, tps=None, counter = None):

        if tps is None:
            tps=60

        self._counter = self._counter+1 if counter is None else counter

        self.close()

        if self._record_image:
            self._movie_writer = imageio.get_writer(
                f"{self._path_base}{self._counter:03d}.mp4",
                fps=tps,
                quality=9,
            )

        self._data = {name_data: [] for name_data in self._data}

    def close(self):
        if self._movie_writer is not None:
            self._movie_writer.close()

        np.savez_compressed(f"{self._path_base}{self._counter:03d}.npz",**self._data)

    def new_entry(self, image, obs, rew, done, info):

        if self._record_image:
            self._movie_writer.append_data(image)

        if self._record_rew:
            self._data["reward"].append(rew)

        if self._record_done:
            self._data["done"].append(done)

        for name_data, (name_obs, transform) in self._obs_map.items():
            self._data[name_data].append(transform(obs[name_obs]))

        for name_data, (name_info, transform) in self._info_map.items():
            self._data[name_data].append(transform(info[name_info]))
