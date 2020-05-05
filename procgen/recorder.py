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


class VecRecorder():

    class Rec():

        def __init__(self, base_name):
            self._base_name = base_name
            self._image_writers = {}
            self._data = {}
            self._closed = False

        def close(self):

            for name, image_writer in self._image_writers.items():
                image_writer.close()
            np.savez_compressed(f"{self._base_name}.npz",**self._data)
            self._closed = True

        def is_closed(self):
            return self._closed

        def new_image_field(self, name=None, tps=None):

            name = '' if name is None else f"_{name}"

            assert name not in self._image_writers
            if tps is None:
                tps = 60

            self._image_writers[name] = imageio.get_writer(
                f"{self._base_name}{name}.mp4",
                fps=tps,
                quality=9,
            )

        def new_data_field(self, name):
            assert name not in self._data
            self._data[name] = []

        def write_image(self, data, name=None):
            name = '' if name is None else f"_{name}"
            self._image_writers[name].append_data(np.array(data))

        def write_data(self, data, name):
            self._data[name].append(np.array(data))


    def __init__(self, record_dir, prefix = None, record_render=True, record_rew=True, record_action=True, record_done=True, continue_counter=True, counter=None):
        self._info_map = {}
        self._obs_map = {}
        self._record_render = record_render
        self._record_rew = record_rew
        self._record_action = record_action
        self._record_done = record_done

        self._recs = {}

        os.makedirs(record_dir, exist_ok=True)
        self._path_base = os.path.join(record_dir, "" if prefix is None else f"{prefix}_")

        assert not ((counter is not None) and (continue_counter==True))
        self._counter = 0
        if continue_counter:
            prev_recordings = sorted(glob.glob(f"{self._path_base}[0-9][0-9][0-9]*.mp4") + glob.glob(f"{self._path_base}[0-9][0-9][0-9].npz"))
            if len(prev_recordings) > 0:
                self._counter = int(prev_recordings[-1][-7:-4])

        elif counter is not None:
            self._counter = counter;


    def new_recording(self, rec_idx):


        if isinstance(rec_idx, (list, tuple, set, np.ndarray)):
            for i, ri in enumerate(rec_idx):
                if isinstance(ri,(bool,np.bool_)):
                    if ri:
                        self.new_recording(i)
                else:
                    self.new_recording(ri)
            return


        if rec_idx in self._recs:
            self._recs[rec_idx].close()

        rec = VecRecorder.Rec(f"{self._path_base}{self._counter:03d}")
        self._counter += 1

        if self._record_render:
            rec.new_image_field()

        if self._record_rew:
            rec.new_data_field('reward')

        if self._record_done:
            rec.new_data_field('done')

        if self._record_action:
            rec.new_data_field('action')

        for name, (no,tr,as_image) in self._obs_map.items():
            if as_image:
                rec.new_image_field(name)
            else:
                rec.new_data_field(name)

        for name, (ni,tr,as_image) in self._info_map.items():
            if as_image:
                rec.new_image_field(name)
            else:
                rec.new_data_field(name)

        self._recs[rec_idx] = rec


    def _check_data_key(self, key):
        if key in self._info_map:
            raise KeyError(f"name already registered in info: {key}")
        if key in self._obs_map:
            raise KeyError(f"name already registered in obs: {key}")
        if self._record_rew and key == "reward":
            raise KeyError(f"name already registered for 'rew' field: {key}")
        if self._record_action and key == "action":
            raise KeyError(f"name already registered for 'action' field: {key}")
        if self._record_done and key == "done":
            raise KeyError(f"name already registered for 'done' field: {key}")

    def record_info_as(self, name_info, name_data, transform=None, as_image=False):
        self._check_data_key(name_data)
        self._info_map[name_data] = (name_info, transform if transform is not None else lambda _:_, as_image)

    def record_obs_as(self, name_obs, name_data, transform=None, as_image=False):
        self._check_data_key(name_data)
        self._obs_map[name_data] = (name_obs, transform if transform is not None else lambda _:_, as_image)

    def close(self, rec_idx=None):

        if rec_idx is None:
            rec_idx = list(self._recs.keys())
        elif not isinstance(rec_idx,(tuple,list,set,np.ndarray)):
            rec_idx = [rec_idx,]

        for ri in rec_idx:
            assert ri in self._recs
            self._recs[ri].close()


    def new_entry(self, rec_idx=None, render=None, obs=None, rew=None, done=None, info=None, action=None):

        if rec_idx is None:
            rec_idx = list(self._recs.keys())
        elif not isinstance(rec_idx,(tuple,list,set,np.ndarray)):
            rec_idx = [rec_idx,]

        # rearranging the rendered image
        if self._record_render:
            render = render.reshape(render.shape[0]//512, 512, -1, 512, render.shape[2]).swapaxes(1,2).reshape(-1, 512, 512, render.shape[2])

        for ri in rec_idx:

            assert ri in self._recs
            assert not self._recs[ri].is_closed()

            if self._record_render:
                self._recs[ri].write_image(render[ri])

            if self._record_rew:
                self._recs[ri].write_data(rew[ri],'reward')

            if self._record_done:
                self._recs[ri].write_data(done[ri],'done')

            if self._record_action:
                self._recs[ri].write_data(action[ri],'action')

            for name_data, (name_obs, transform, as_image) in self._obs_map.items():
                if as_image:
                    self._recs[ri].write_image(transform(obs[name_obs][ri,...]),name_data)
                else:
                    self._recs[ri].write_data(transform(obs[name_obs][ri,...]),name_data)

            for name_data, (name_info, transform, as_image) in self._info_map.items():
                if as_image:
                    self._recs[ri].write_image(transform(info[ri][name_info]),name_data)
                else:
                    self._recs[ri].write_data(transform(info[ri][name_info]),name_data)



class SingleRecorder(VecRecorder):

    def __init__(self, record_dir, prefix = None, record_render=True, record_rew=True, record_action=True, record_done=True, continue_counter=True, counter=None):

        VecRecorder.__init__(self, record_dir, prefix, record_render, record_rew, record_action, record_done, continue_counter, counter)

    def new_recording(self):
        VecRecorder.new_recording(self,0)

    def close(self):
        VecRecorder.new_recording(self,0)

    def new_entry(self, render=None, obs=None, rew=None, done=None, info=None, action=None):

        rec = self._recs[0]

        assert not rec.is_closed()

        if self._record_render:
            rec.write_image(render)

        if self._record_rew:
            rec.write_data(rew,'reward')

        if self._record_done:
            rec.write_data(done,'done')

        if self._record_action:
            rec.write_data(action,'action')

        for name_data, (name_obs, transform, as_image) in self._obs_map.items():
            if as_image:
                rec.write_image(transform(obs[name_obs]),name_data)
            else:
                rec.write_data(transform(obs[name_obs]),name_data)

        for name_data, (name_info, transform, as_image) in self._info_map.items():
            if as_image:
                rec.write_image(transform(info[name_info]),name_data)
            else:
                rec.write_data(transform(info[name_info]),name_data)



class Recorder():

    def __init__(self, record_dir, num_envs=1, prefix = None, record_image=True, record_rew=True, record_done=True, continue_counter=True):
        self._movie_writer = None
        self._data = {}
        self._info_map = {}
        self._obs_map = {}
        self._record_image = record_image
        self._record_rew = record_rew
        self._record_done = record_done

        self.closed = True

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

        if not self.closed:
            self.close()

        if tps is None:
            tps=60

        self._counter = self._counter+1 if counter is None else counter


        if self._record_image:
            self._movie_writer = imageio.get_writer(
                f"{self._path_base}{self._counter:03d}.mp4",
                fps=tps,
                quality=9,
            )

        self._data = {name_data: [] for name_data in self._data}

    def close(self):

        if self.closed:
            return

        self.closed = True

        if self._movie_writer is not None:
            self._movie_writer.close()

        np.savez_compressed(f"{self._path_base}{self._counter:03d}.npz",**self._data)

    def new_entry(self, image, obs, rew, done, info, action =None):

        if closed:
            return

        if self._record_image:
            self._movie_writer.append_data(np.array(image))

        if self._record_rew:
            self._data["reward"].append(np.array(rew))

        if self._record_done:
            self._data["done"].append(np.array(done))

        for name_data, (name_obs, transform) in self._obs_map.items():
            self._data[name_data].append(transform(np.array(obs[name_obs])))

        for name_data, (name_info, transform) in self._info_map.items():
            self._data[name_data].append(transform(np.array(info[name_info])))

        if action is not None:
            if "action" not in self._data:
                self._data["action"] = []
            self._data["action"].append(action)
