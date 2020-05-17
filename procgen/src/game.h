#pragma once

/*

Base class used for all games, all games must inherit from this

*/

#include <QtGui/QPainter>
#include <memory>
#include <functional>
#include <vector>
#include <variant>

#include "libenv.h"

#include "entity.h"
#include "randgen.h"
#include "resources.h"
#include "object-ids.h"
#include "game-registry.h"

// We want all games to have same observation space. So all these
// constants here related to observation space are constants forever.
const int RES_W = 64;
const int RES_H = 64;

const int RENDER_RES = 512;

void bgr32_to_rgb888(void *dst_rgb888, void *src_bgr32, int w, int h);

class VecOptions;

enum DistributionMode {
    EasyMode = 0,
    HardMode = 1,
    ExtremeMode = 2,
    MemoryMode = 10,
};

struct StepData {
    float reward = 0.0f;
    bool done = false;
    bool level_complete = false;
};


struct GameOptionBase {
  enum libenv_dtype dtype;
};

template<typename T>
struct GameOption : public GameOptionBase {
  std::unique_ptr<T[]> data;
  const uint32_t size;

  GameOption(const uint32_t _size): size(_size) {
    if constexpr (std::is_same_v<T,uint8_t>){
      dtype = LIBENV_DTYPE_UINT8;
    }else if constexpr (std::is_same_v<T,int32_t>){
      dtype = LIBENV_DTYPE_INT32;
    }else if constexpr (std::is_same_v<T,float>){
      dtype = LIBENV_DTYPE_FLOAT32;
    }else{
      fassert(false);
    }
    data = std::make_unique<T[]>(size);
  }

  void assign(T value){
    data[0] = value;
  }

  T get(){
    return data[0];
  }

  void assign(T* value, uint32_t assign_size){
    fassert(size >= assign_size);
    std::cout << "Not yet implemented" << std::endl;
    fassert(false);
  }

  void assign(T* value, uint32_t assign_size, uint32_t offset){
    fassert(size >= assign_size+offset);
    std::cout << "Not yet implemented" << std::endl;
    fassert(false);
  }

};

struct GameOptions {
    bool paint_vel_info = false;
    bool use_generated_assets = true;
    bool center_agent = false;
    int debug_mode = 0;
    DistributionMode distribution_mode = HardMode;
    bool use_sequential_levels = false;

    // coinrun_old
    bool use_easy_jump = false;
    int plain_assets = 0;
    int physics_mode = 0;

    std::map<std::string, std::shared_ptr<GameOptionBase>> opts;

    template<typename T>
    void register_option(std::string name, uint32_t count, T* default_value){
      std::cout << "Not yet implemented" << std::endl;
      fassert(false);
    }

    template<typename T>
    void register_option(std::string name, T default_value){
      fassert(opts.find(name) == opts.end());

      auto gopt = std::make_shared<GameOption<T>>(1);
      gopt->assign(default_value);
      opts[name] = gopt;
    }

    template<typename T>
    void assign(std::string name, T value){
      auto gopt_it = opts.find(name);
      fassert(gopt_it != opts.end());
      if constexpr (std::is_same_v<T,uint8_t>){
        gopt_it->second->dtype = LIBENV_DTYPE_UINT8;
      }else if constexpr (std::is_same_v<T,int32_t>){
        gopt_it->second->dtype = LIBENV_DTYPE_INT32;
      }else if constexpr (std::is_same_v<T,float>){
        gopt_it->second->dtype = LIBENV_DTYPE_FLOAT32;
      }else{
        fassert(false);
      }
      std::static_pointer_cast<GameOption<T>>(gopt_it->second)->assign(value);
    }

    template <typename T>
    T get(std::string name){
      auto gopt_it = opts.find(name);
      fassert(gopt_it != opts.end());
      if constexpr (std::is_same_v<T,uint8_t>){
        gopt_it->second->dtype = LIBENV_DTYPE_UINT8;
      }else if constexpr (std::is_same_v<T,int32_t>){
        gopt_it->second->dtype = LIBENV_DTYPE_INT32;
      }else if constexpr (std::is_same_v<T,float>){
        gopt_it->second->dtype = LIBENV_DTYPE_FLOAT32;
      }else{
        fassert(false);
      }
      return std::static_pointer_cast<GameOption<T>>(gopt_it->second)->get();
    }

    template<typename T>
    bool exists(std::string name){
      auto gopt_it = opts.find(name);
      if (opts.find(name) == opts.end()){
        return false;
      }
      if constexpr (std::is_same_v<T,uint8_t>){
        return gopt_it->second->dtype == LIBENV_DTYPE_UINT8;
      }else if constexpr (std::is_same_v<T,int32_t>){
        return gopt_it->second->dtype == LIBENV_DTYPE_INT32;
      }else if constexpr (std::is_same_v<T,float>){
        return gopt_it->second->dtype == LIBENV_DTYPE_FLOAT32;
      }else{
        fassert(false);
      }
    }

};

struct GameSpaceBuffer{
  void *buffer = 0;
  const libenv_space *space = 0;

  template<typename T>
  void assign(T value){
    if constexpr (std::is_same_v<T,uint8_t>){
      fassert(space->dtype == LIBENV_DTYPE_UINT8);
    }else if constexpr (std::is_same_v<T,int32_t>){
      fassert(space->dtype == LIBENV_DTYPE_INT32);
    }else if constexpr (std::is_same_v<T,float>){
      fassert(space->dtype == LIBENV_DTYPE_FLOAT32);
    }else{
      fassert(false);
    }

    *(T*)(buffer) = value;
  }

  template<typename T=void>
  T* ptr(){
    if constexpr (std::is_same_v<T,uint8_t>){
      fassert(space->dtype == LIBENV_DTYPE_UINT8);
    }else if constexpr (std::is_same_v<T,int32_t>){
      fassert(space->dtype == LIBENV_DTYPE_INT32);
    }else if constexpr (std::is_same_v<T,float>){
      fassert(space->dtype == LIBENV_DTYPE_FLOAT32);
    }else{
      fassert(false);
    }

    return (T*) buffer;
  }
};

class Game {
  public:
    GameOptions options;

    bool grid_step = false;
    int level_seed_low = 0;
    int level_seed_high = 1;
    int game_type = 0;
    int game_n = 0;

    RandGen level_seed_rand_gen;
    RandGen rand_gen;

    StepData step_data;
    int action = 0;

    int timeout = 0;

    int current_level_seed = 0;
    int episodes_remaining = 0;
    // bool episode_done = false;

    // float last_ep_reward = 0.0f;
    int last_reward_timer = 0;
    float last_reward = 0.0f;
    int default_action = 0;

    int fixed_asset_seed = 0;

    uint32_t render_buf[RES_W * RES_H];

    int cur_time = 0;

    bool is_waiting_for_step = false;



    // pointers to buffers where we should put step data
    // these are set by step_async
    std::vector<void *> obs_bufs;
    std::vector<void *> info_bufs;
    float *reward_ptr = nullptr;
    uint8_t *done_ptr = nullptr;
    std::map<std::string, GameSpaceBuffer> info_buffers;
    std::map<std::string, GameSpaceBuffer> obs_buffers;

    Game();
    void reset();
    bool was_reset();
    void step();
    void render_to_buf(void *buf, int w, int h, bool antialias);
    void parse_options(std::string name, VecOptions opt_vec);

    virtual ~Game() = 0;
    virtual void game_init() = 0;
    virtual void game_reset() = 0;
    virtual void game_step() = 0;
    virtual void game_draw(QPainter &p, const QRect &rect) = 0;

    void register_info_buffer(std::string name);

    void register_obs_buffer(std::string name);

    void connect_buffer(std::map<std::string, GameSpaceBuffer> &buffer_map, const std::vector<struct libenv_space> &spaces, const std::vector<void *> &buffer);
    void connect_info_buffer(const std::vector<struct libenv_space> &spaces, const std::vector<void *> &buffer);
    void connect_obs_buffer(const std::vector<struct libenv_space> &spaces, const std::vector<void *> &buffer);

    template <typename T>
    void assign_to_buffer(const std::string name, std::map<std::string,GameSpaceBuffer> &buffers, T value){
      auto bptr = buffers.find(name);
      fassert(bptr != buffers.end());
      if(bptr->second.space == 0){
        return;
      }
      bptr->second.assign<T>(value);
    }

    template<typename T = void>
    T* point_to_buffer(std::map<std::string,GameSpaceBuffer> &buffers, const std::string name){
      auto bptr = buffers.find(name);
      fassert(bptr != buffers.end());
      if(bptr->second.space == 0){
        return 0;
      }
      return bptr->second.ptr<T>();
    }

    void assign_to_info(const std::string name, uint8_t value);
    void assign_to_info(const std::string name, int32_t value);
    void assign_to_info(const std::string name, float value);

    template<typename T = void>
    T* point_to_info(const std::string name){
      return point_to_buffer<T>(info_buffers, name);
    }

    void assign_to_obs(const std::string name, uint8_t value);
    void assign_to_obs(const std::string name, int32_t value);
    void assign_to_obs(const std::string name, float value);

    template<typename T = void>
    T* point_to_obs(const std::string name){
      return point_to_buffer<T>(obs_buffers, name);
    }

    int get_num_episodes_done();

  private:
    bool _was_reset = false;
    int reset_count = 0;
    int num_episodes_done = 0;
    float total_reward = 0.0f;
};
