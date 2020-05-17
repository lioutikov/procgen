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
from .scalarize import Scalarize
from .gym_registration import RemoveDictObs

class RecorderWrapper():

    def __init__(self, record_dir, prefix = None, record_render=True, record_rew=True, record_action=True, record_done=True, continue_counter=True, counter=None):
        self._info_map = {}
        self._obs_map = {}
        self._record_dir = record_dir
        self._prefix = prefix
        self._record_render = record_render
        self._record_rew = record_rew
        self._record_action = record_action
        self._record_done = record_done
        self._continue_counter = continue_counter;
        self._counter = counter;

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

    def __call__(self, env):

        if isinstance(env, Scalarize):
            env = ScalarRecWrapper(env, self._record_dir, self._prefix, self._record_render, self._record_rew, self._record_action, self._record_done, self._continue_counter, self._counter)
            for name_data, (name_obs, transform, as_image) in self._obs_map.items():
                env.record_obs_as(name_data, name_obs, transform, as_image)

        elif isinstance(env, RemoveDictObs):
            env = GymRecWrapper(env, self._record_dir, self._prefix, self._record_render, self._record_rew, self._record_action, self._record_done, self._continue_counter, self._counter)
            for name_data, (name_obs, transform, as_image) in self._obs_map.items():
                if name_obs == 'obs':
                    env.record_obs_as(name_data, name_obs, transform, as_image)
                else:
                    print(f"Only 'obs' keyword is supported for gym evironments.Exiting")
                    exit(-1)

        else:
            env = VectorRecWrapper(env, self._record_dir, self._prefix, self._record_render, self._record_rew, self._record_action, self._record_done, self._continue_counter, self._counter)
            for name_data, (name_obs, transform, as_image) in self._obs_map.items():
                env.record_obs_as(name_data, name_obs, transform, as_image)

        for name_data, (name_info, transform, as_image) in self._info_map.items():
            env.record_info_as(name_data, name_info, transform, as_image)

        return env


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


class VectorRecWrapper():

    def __init__(self, env, record_dir, prefix = None, record_render=True, record_rew=True, record_action=True, record_done=True, continue_counter=True, counter=None):
        self._info_map = {}
        self._obs_map = {}
        self._record_render = record_render
        self._record_rew = record_rew
        self._record_action = record_action
        self._record_done = record_done

        self.env = env

        self.observation_space = self.env.observation_space
        self.action_space = self.env.action_space
        self.metadata = self.env.metadata
        self.spec = self.env.spec if hasattr(self.env, "spec") else None
        self.reward_range = self.env.reward_range if hasattr(self.env, "reward_range") else None

        self.recs = [None]*self.env.num_envs

        os.makedirs(record_dir, exist_ok=True)
        self._path_base = os.path.join(record_dir, "" if prefix is None else f"{prefix}_")

        assert not ((counter is not None) and (continue_counter==True))
        self._counter = 0
        if continue_counter:
            prev_recordings = sorted(glob.glob(f"{self._path_base}[0-9][0-9][0-9][0-9]*.mp4") + glob.glob(f"{self._path_base}[0-9][0-9][0-9][0-9].npz"))
            if len(prev_recordings) > 0:
                self._counter = int(prev_recordings[-1][-7:-4])+1

        elif counter is not None:
            self._counter = counter;

    def new_rec(self):

        rec = Rec(f"{self._path_base}{self._counter:04d}")
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

        return rec

    def add_render(self, rec, render):
        if self._record_render:
            rec.write_image(render)

    def add_obs(self, rec, obs):
        for name_data, (name_obs, transform, as_image) in self._obs_map.items():
            if as_image:
                rec.write_image(transform(obs[name_obs]),name_data)
            else:
                rec.write_data(transform(obs[name_obs]),name_data)

    def add_action(self, rec, action):
        if self._record_action:
            rec.write_data(action,'action')

    def add_rew(self, rec, rew):
        if self._record_rew:
            rec.write_data(rew,'reward')

    def add_done(self, rec, done):
        if self._record_done:
            rec.write_data(done,'done')

    def add_info(self, rec, info):
        for name_data, (name_info, transform, as_image) in self._info_map.items():
            if as_image:
                rec.write_image(transform(info[name_info]),name_data)
            else:
                rec.write_data(transform(info[name_info]),name_data)


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


    def _record_reset_for_rec_at_i(self, i, obs, render):

        if self.recs[i] is not None:
            self.recs[i].close()
        self.recs[i] = self.new_rec()

        self.add_obs(self.recs[i],obs)
        self.add_render(self.recs[i],render)


    def reset(self, reset_game=None):

        print(self.env)
        obs = self.env.reset(reset_game)
        _render = self.env.get_images()

        for i in range(len(self.recs)):
            if (reset_game is None) or reset_game[i]:
                _obs_i = {k:v[i,...] for k,v in obs.items()}
                self._record_reset_for_rec_at_i(i,_obs_i,_render[i])

        return obs

    def _record_step_for_rec_at_i(self, i, render, action, obs, rew,  done, info):

        rec = self.recs[i]
        self.add_render(rec, render)
        self.add_action(rec, action)
        self.add_rew(rec, rew)
        self.add_info(rec, info)
        self.add_done(rec, done)
        self.add_obs(rec, obs)

        if done:
            rec.close()
            self.recs[i] = None

    def step(self, actions):

        obs, rew, done ,info = self.env.step(actions)
        _render = self.env.get_images()

        for i in range(len(self.recs)):
            if self.recs[i] is not None:
                _obs_i = {k:v[i,...] for k,v in obs.items()}
                self._record_step_for_rec_at_i(i, _render[i], actions[i], _obs_i, rew[i], done[i], info[i])

        return obs, rew, done ,info

    def render(self, mode=None):
        return self.env.render(mode)

    def get_images(self):
        return self.env.get_images()

    def close(self):
        for rec in self.recs:
            if rec is not None:
                rec.close()
        return self.env.close()

    def seed(self, seed=None):
        return self.env.seed(seed)

    @property
    def unwrapped(self):
        return self

    def __repr__(self):
        return f"<VectorRecWrapper venv={self.env}>"

class ScalarRecWrapper(VectorRecWrapper):

    def reset(self):

        obs = self.env.reset()
        render = self.env.get_images()
        self._record_reset_for_rec_at_i(0,obs,render)

        return obs


    def step(self, action):
        obs, rew, done ,info = self.env.step(action)
        _render = self.env.get_images()

        if self.recs[0] is not None:
            self._record_step_for_rec_at_i(0, _render, action, obs, rew, done, info)

        return obs, rew, done ,info

    def __repr__(self):
        return f"<ScalarRecWrapper venv={self.env}>"


class GymRecWrapper(ScalarRecWrapper):

    def reset(self):

        obs = self.env.reset()
        _render = self.env.get_images()
        _obs = {'obs':obs}
        self._record_reset_for_rec_at_i(0,_obs,_render)

        return obs


    def step(self, action):
        obs, rew, done ,info = self.env.step(action)
        _render = self.env.get_images()
        _obs = {'obs':obs}

        if self.recs[0] is not None:
            self._record_step_for_rec_at_i(0, _render, action, _obs, rew, done, info)

        return obs, rew, done ,info

    def __repr__(self):
        return f"<GymRecWrapper venv={self.env}>"




#
# class VecRecorder():
#
#     class Rec():
#
#         def __init__(self, base_name):
#             self._base_name = base_name
#             self._image_writers = {}
#             self._data = {}
#             self._closed = False
#
#         def close(self):
#
#             for name, image_writer in self._image_writers.items():
#                 image_writer.close()
#             np.savez_compressed(f"{self._base_name}.npz",**self._data)
#             self._closed = True
#
#         def is_closed(self):
#             return self._closed
#
#         def new_image_field(self, name=None, tps=None):
#
#             name = '' if name is None else f"_{name}"
#
#             assert name not in self._image_writers
#             if tps is None:
#                 tps = 60
#
#             self._image_writers[name] = imageio.get_writer(
#                 f"{self._base_name}{name}.mp4",
#                 fps=tps,
#                 quality=9,
#             )
#
#         def new_data_field(self, name):
#             assert name not in self._data
#             self._data[name] = []
#
#         def write_image(self, data, name=None):
#             name = '' if name is None else f"_{name}"
#             self._image_writers[name].append_data(np.array(data))
#
#         def write_data(self, data, name):
#             self._data[name].append(np.array(data))
#
#
#     def __init__(self, num_recs, record_dir, prefix = None, record_render=True, record_rew=True, record_action=True, record_done=True, continue_counter=True, counter=None):
#         self._info_map = {}
#         self._obs_map = {}
#         self._record_render = record_render
#         self._record_rew = record_rew
#         self._record_action = record_action
#         self._record_done = record_done
#         self._num_recs = num_recs
#
#         self._recs = [None for _ in range(num_recs)]
#
#         os.makedirs(record_dir, exist_ok=True)
#         self._path_base = os.path.join(record_dir, "" if prefix is None else f"{prefix}_")
#
#         assert not ((counter is not None) and (continue_counter==True))
#         self._counter = 0
#         if continue_counter:
#             prev_recordings = sorted(glob.glob(f"{self._path_base}[0-9][0-9][0-9]*.mp4") + glob.glob(f"{self._path_base}[0-9][0-9][0-9].npz"))
#             if len(prev_recordings) > 0:
#                 self._counter = int(prev_recordings[-1][-7:-4])+1
#
#         elif counter is not None:
#             self._counter = counter;
#
#     def new_recording(self, reopen):
#
#         for rec_idx, do_reopen in enumerate(reopen):
#
#             if not do_reopen:
#                 continue
#
#             assert (self._recs[rec_idx] is None) or self._recs[rec_idx].is_closed()
#             rec = VecRecorder.Rec(f"{self._path_base}{self._counter:03d}")
#             self._counter += 1
#
#             if self._record_render:
#                 rec.new_image_field()
#
#             if self._record_rew:
#                 rec.new_data_field('reward')
#
#             if self._record_done:
#                 rec.new_data_field('done')
#
#             if self._record_action:
#                 rec.new_data_field('action')
#
#             for name, (no,tr,as_image) in self._obs_map.items():
#                 if as_image:
#                     rec.new_image_field(name)
#                 else:
#                     rec.new_data_field(name)
#
#             for name, (ni,tr,as_image) in self._info_map.items():
#                 if as_image:
#                     rec.new_image_field(name)
#                 else:
#                     rec.new_data_field(name)
#
#             self._recs[rec_idx] = rec
#
#
#     def _check_data_key(self, key):
#         if key in self._info_map:
#             raise KeyError(f"name already registered in info: {key}")
#         if key in self._obs_map:
#             raise KeyError(f"name already registered in obs: {key}")
#         if self._record_rew and key == "reward":
#             raise KeyError(f"name already registered for 'rew' field: {key}")
#         if self._record_action and key == "action":
#             raise KeyError(f"name already registered for 'action' field: {key}")
#         if self._record_done and key == "done":
#             raise KeyError(f"name already registered for 'done' field: {key}")
#
#     def record_info_as(self, name_info, name_data, transform=None, as_image=False):
#         self._check_data_key(name_data)
#         self._info_map[name_data] = (name_info, transform if transform is not None else lambda _:_, as_image)
#
#     def record_obs_as(self, name_obs, name_data, transform=None, as_image=False):
#         self._check_data_key(name_data)
#         self._obs_map[name_data] = (name_obs, transform if transform is not None else lambda _:_, as_image)
#
#     def close(self, doclose, render=None, obs=None, info=None):
#
#         for ri, (rec, dclose) in enumerate(zip(self._recs, doclose)):
#             if not dclose:
#                 continue
#
#             obs_ri = None
#             if obs is not None:
#                 obs_ri = {k:v[ri,...] for k,v in obs.items()}
#
#             self._new_entry_for_rec(rec, render= None if render is None else render[ri], obs=obs_ri, rew=None, done=dclose, info =None if info is None else info[ri], action=None)
#
#             rec.close()
#
#     def _new_entry_for_rec(self, rec, render=None, obs=None, rew=None, done=None, info=None, action=None):
#
#         if rec.is_closed():
#             return
#
#         if self._record_render:
#             rec.write_image(render)
#
#         if rew is not None:
#             if self._record_rew:
#                 rec.write_data(rew,'reward')
#
#         if done is not None:
#             if self._record_done:
#                 rec.write_data(done,'done')
#
#         if action is not None:
#             if self._record_action:
#                 rec.write_data(action,'action')
#
#         for name_data, (name_obs, transform, as_image) in self._obs_map.items():
#             if as_image:
#                 rec.write_image(transform(obs[name_obs]),name_data)
#             else:
#                 rec.write_data(transform(obs[name_obs]),name_data)
#
#         if info is not None:
#             for name_data, (name_info, transform, as_image) in self._info_map.items():
#                 if as_image:
#                     rec.write_image(transform(info[name_info]),name_data)
#                 else:
#                     rec.write_data(transform(info[name_info]),name_data)
#
#
#     def new_entry(self, render=None, obs=None, rew=None, done=None, info=None, action=None):
#
#
#         for ri, rec in enumerate(self._recs):
#
#             obs_ri = {k:v[ri,...] for k,v in obs.items()}
#
#             self._new_entry_for_rec(rec, render=render[ri], obs=obs_ri, rew=None if rew is None else rew[ri], done=done[ri], info=None if info is None else info[ri], action=None if action is None else action[ri])
#
#             # if rec.is_closed():
#             #     continue
#             #
#             # if self._record_render:
#             #     rec.write_image(render[ri])
#             #
#             # if rew is not None:
#             #     if self._record_rew:
#             #         rec.write_data(rew[ri],'reward')
#             #
#             # if done is not None:
#             #     if self._record_done:
#             #         rec.write_data(done[ri],'done')
#             #
#             # if action is not None:
#             #     if self._record_action:
#             #         rec.write_data(action[ri],'action')
#             #
#             # for name_data, (name_obs, transform, as_image) in self._obs_map.items():
#             #     if as_image:
#             #         rec.write_image(transform(obs[name_obs][ri,...]),name_data)
#             #     else:
#             #         rec.write_data(transform(obs[name_obs][ri,...]),name_data)
#             #
#             # if info is not None:
#             #     for name_data, (name_info, transform, as_image) in self._info_map.items():
#             #         if as_image:
#             #             rec.write_image(transform(info[ri][name_info]),name_data)
#             #         else:
#             #             rec.write_data(transform(info[ri][name_info]),name_data)
#
#
#
# class SingleRecorder(VecRecorder):
#
#     def __init__(self, record_dir, prefix = None, record_render=True, record_rew=True, record_action=True, record_done=True, continue_counter=True, counter=None):
#
#         VecRecorder.__init__(self, 1, record_dir, prefix, record_render, record_rew, record_action, record_done, continue_counter, counter)
#
#     def new_recording(self):
#         VecRecorder.new_recording(self,[True])
#         self._recs[0].new_data_field('_')
#         self._recs[0].write_data('_','_')
#
#     def close(self, render=None, obs=None, info=None):
#
#         rec = self._recs[0]
#         if rec.is_closed():
#             return
#
#         self._new_entry_for_rec(rec, render= None if render is None else render, obs=None if obs is None else obs, rew=None, done=True, info =None if info is None else info, action=None)
#         rec.close()
#
#
#
#     def new_entry(self, render=None, obs=None, rew=None, done=None, info=None, action=None):
#
#         rec = self._recs[0]
#
#         if rec.is_closed():
#             return
#
#         if self._record_render:
#             rec.write_image(render)
#
#         if rew is not None:
#             if self._record_rew:
#                 rec.write_data(rew,'reward')
#
#         if done is not None:
#             if self._record_done:
#                 rec.write_data(done,'done')
#
#         if action is not None:
#             if self._record_action:
#                 rec.write_data(action,'action')
#
#         for name_data, (name_obs, transform, as_image) in self._obs_map.items():
#             if as_image:
#                 rec.write_image(transform(obs[name_obs]),name_data)
#             else:
#                 rec.write_data(transform(obs[name_obs]),name_data)
#
#         if info is not None:
#             for name_data, (name_info, transform, as_image) in self._info_map.items():
#                 if as_image:
#                     rec.write_image(transform(info[name_info]),name_data)
#                 else:
#                     rec.write_data(transform(info[name_info]),name_data)
#
#
#
# class Recorder():
#
#     def __init__(self, record_dir, num_envs=1, prefix = None, record_image=True, record_rew=True, record_done=True, continue_counter=True):
#         self._movie_writer = None
#         self._data = {}
#         self._info_map = {}
#         self._obs_map = {}
#         self._record_image = record_image
#         self._record_rew = record_rew
#         self._record_done = record_done
#
#         self.closed = True
#
#         os.makedirs(record_dir, exist_ok=True)
#         self._path_base = os.path.join(record_dir, "" if prefix is None else f"{prefix}_")
#
#         counter = 0
#         if continue_counter:
#             prev_recordings = sorted(glob.glob(f"{self._path_base}[0-9][0-9][0-9].mp4") + glob.glob(f"{self._path_base}[0-9][0-9][0-9].npz"))
#
#             if len(prev_recordings) > 0:
#                 counter = int(prev_recordings[-1][-7:-4])
#
#         self._counter = counter;
#
#         if self._record_rew:
#             self._data["reward"] = []
#         if self._record_done:
#             self._data["done"] = []
#
#     def record_info_as(self, name_info, name_data, transform=None):
#         if name_data in self._data:
#             raise KeyError(f"name already registered: {name_data}")
#         self._data[name_data] = []
#         self._info_map[name_data] = (name_info, transform if transform is not None else lambda _:_)
#
#     def record_obs_as(self, name_obs, name_data, transform=None):
#         if name_data in self._data:
#             raise KeyError(f"name already registered: {name_data}")
#         self._data[name_data] = []
#         self._obs_map[name_data] = (name_obs, transform if transform is not None else lambda _:_)
#
#     def new_recording(self, tps=None, counter = None):
#
#         if not self.closed:
#             self.close()
#
#         if tps is None:
#             tps=60
#
#         self._counter = self._counter+1 if counter is None else counter
#
#
#         if self._record_image:
#             self._movie_writer = imageio.get_writer(
#                 f"{self._path_base}{self._counter:03d}.mp4",
#                 fps=tps,
#                 quality=9,
#             )
#
#         self._data = {name_data: [] for name_data in self._data}
#
#     def close(self):
#
#         if self.closed:
#             return
#
#         self.closed = True
#
#         if self._movie_writer is not None:
#             self._movie_writer.close()
#
#         np.savez_compressed(f"{self._path_base}{self._counter:03d}.npz",**self._data)
#
#     def new_entry(self, image, obs, rew, done, info, action =None):
#
#         if closed:
#             return
#
#         if self._record_image:
#             self._movie_writer.append_data(np.array(image))
#
#         if self._record_rew:
#             self._data["reward"].append(np.array(rew))
#
#         if self._record_done:
#             self._data["done"].append(np.array(done))
#
#         for name_data, (name_obs, transform) in self._obs_map.items():
#             self._data[name_data].append(transform(np.array(obs[name_obs])))
#
#         for name_data, (name_info, transform) in self._info_map.items():
#             self._data[name_data].append(transform(np.array(info[name_info])))
#
#         if action is not None:
#             if "action" not in self._data:
#                 self._data["action"] = []
#             self._data["action"].append(action)
