#include "../basic-abstract-game.h"
#include "../assetgen.h"
#include <set>
#include <queue>
#include "../mazegen.h"
#include "../cpp-utils.h"

const int KEY = 1;
const int LOCKED_DOOR = 5;
const int EXIT = 9;
const int KEY_ON_RING = 11;
const int WATER = 20;
const int FIRE = 21;

class HeistPPGame : public BasicAbstractGame {


    RandGen placement_rand_gen;

  public:
    std::shared_ptr<MazeGen> maze_gen;
    int world_dim, num_keys, num_doors;
    std::vector<bool> has_keys;

    float wall_chance;
    float water_chance;
    float fire_chance;

    bool with_grid_steps;

    float completion_bonus;
    float fire_bonus;
    float water_bonus;
    float action_bonus;

    const std::map<int,uint8_t> asset_to_state = {
      {SPACE, 0},
      {KEY, 11},
      {KEY+1, 12},
      {KEY+2, 13},
      {LOCKED_DOOR, 21},
      {LOCKED_DOOR+1, 22},
      {LOCKED_DOOR+2, 23},
      {EXIT, 30},
      {WATER, 41},
      {FIRE, 42},
      {WALL_OBJ, 50}
    };

    HeistPPGame()
        : BasicAbstractGame() {
        maze_gen = nullptr;
        has_useful_vel_info = false;

        main_width = 20;
        main_height = 20;

        out_of_bounds_object = WALL_OBJ;
        visibility = 8.0;

        register_info_buffer("state");
        register_obs_buffer("state");

        options.register_option<int32_t>("world_dim",5.0);

        options.register_option<float>("wall_chance",1.0);
        options.register_option<float>("water_chance",0.0);
        options.register_option<float>("fire_chance",0.0);

        options.register_option<uint8_t>("with_grid_steps",1);

        options.register_option<int32_t>("num_keys",0);
        options.register_option<int32_t>("num_doors",0);

        options.register_option<float>("completion_bonus",10.0);
        options.register_option<float>("fire_bonus",-5.0);
        options.register_option<float>("water_bonus",-2.0);
        options.register_option<float>("action_bonus",-1.0);
        options.register_option<int32_t>("agent_cell",-1);
        options.register_option<int32_t>("diamond_cell",-1);
        options.register_option<int32_t>("placement_seed",11);
    }

    void load_background_images() override {
        main_bg_images_ptr = &topdown_backgrounds;
    }

    void asset_for_type(int type, std::vector<QString> &names) override {
        if (type == WALL_OBJ) {
            names.push_back("kenney/Ground/Dirt/dirtCenter.png");
        } else if (type == EXIT) {
            names.push_back("misc_assets/gemYellow.png");
        } else if (type == PLAYER) {
            names.push_back("misc_assets/spaceAstronauts_008.png");
        } else if (type == KEY || type == KEY_ON_RING) {
            names.push_back("misc_assets/keyBlue.png");
            names.push_back("misc_assets/keyGreen.png");
            names.push_back("misc_assets/keyRed.png");
        } else if (type == LOCKED_DOOR) {
            names.push_back("misc_assets/lock_blue.png");
            names.push_back("misc_assets/lock_green.png");
            names.push_back("misc_assets/lock_red.png");
        } else if (type == WATER) {
            names.push_back("misc_assets/spaceEffect1_blue.png");
        } else if (type == FIRE) {
            names.push_back("misc_assets/towerDefense_tile295.png");
        }
    }

    bool use_block_asset(int type) override {
        return BasicAbstractGame::use_block_asset(type) || (type == WALL_OBJ) || (type == LOCKED_DOOR);
    }

    bool is_blocked(const std::shared_ptr<Entity> &src, int target, bool is_horizontal) override {
        if (target == LOCKED_DOOR){
          return true;
        }
        return BasicAbstractGame::is_blocked(src, target, is_horizontal);
    }

    bool is_blocked_ents(const std::shared_ptr<Entity> &src, const std::shared_ptr<Entity> &target, bool is_horizontal) override {
        if (target->type == LOCKED_DOOR){
            return !has_keys[target->image_theme];
        }

        return BasicAbstractGame::is_blocked_ents(src, target, is_horizontal);
    }

    bool should_draw_entity(const std::shared_ptr<Entity> &entity) override {
        if (entity->type == KEY_ON_RING)
            return has_keys[entity->image_theme];

        return BasicAbstractGame::should_draw_entity(entity);
    }

    void handle_agent_collision(const std::shared_ptr<Entity> &obj) override {
        BasicAbstractGame::handle_agent_collision(obj);

        if (obj->type == EXIT) {
            step_data.done = true;
            step_data.reward += completion_bonus;
            step_data.level_complete = true;
        } else if (obj->type == KEY) {
            obj->will_erase = true;
            has_keys[obj->image_theme] = true;

            //--- UGLY HACK BUT CURRENTLY REQUIRED FOR grid_step
            if (grid_step){
              for (auto entity :entities){
                if (entity->type != LOCKED_DOOR){
                  continue;
                }
                if (entity->image_theme != obj->image_theme){
                  continue;
                }
                set_obj(int(entity->x), int(entity->y), SPACE);
              }
            }
            //---

        } else if (obj->type == LOCKED_DOOR) {
            int door_num = obj->image_theme;
            if (has_keys[door_num]) {
                obj->will_erase = true;
            }
        }else if (obj->type == WATER) {
          step_data.reward += water_bonus;
        }else if (obj->type == FIRE) {
          step_data.reward += fire_bonus;
        }
    }

    void choose_world_dim() override {

        maxspeed = .75;

        world_dim = options.get<int32_t>("world_dim");
        main_width = world_dim;
        main_height = world_dim;
    }

    void game_init() override {
      BasicAbstractGame::game_init();

      wall_chance = options.get<float>("wall_chance");
      water_chance = options.get<float>("water_chance");
      fire_chance = options.get<float>("fire_chance");

      num_keys = options.get<int32_t>("num_keys");
      num_doors = options.get<int32_t>("num_doors");
      fassert(num_keys >= num_doors);
      fassert(num_keys <= 3);
      fassert(num_doors <= 3);

      float sum_chance = wall_chance+water_chance+fire_chance;
      if (sum_chance > 1.0){
        wall_chance /= sum_chance;
        water_chance /= sum_chance;
        fire_chance /= sum_chance;
      }
      water_chance += wall_chance;
      fire_chance += water_chance;


      with_grid_steps = (bool) options.get<uint8_t>("with_grid_steps");

      completion_bonus = options.get<float>("completion_bonus");
      fire_bonus = options.get<float>("fire_bonus");
      water_bonus = options.get<float>("water_bonus");
      action_bonus = options.get<float>("action_bonus");

      placement_rand_gen.seed(options.get<float>("placement_seed"));
    }

    void game_reset() override {
        BasicAbstractGame::game_reset();

        options.center_agent = options.distribution_mode == MemoryMode;

        grid_step = with_grid_steps;

        has_keys.clear();

        for (int i = 0; i < 3; i++) {
            has_keys.push_back(false);
        }

        int maze_dim = world_dim;

        float maze_scale = main_height / (world_dim * 1.0);

        agent->rx = .375 * maze_scale;
        agent->ry = .375 * maze_scale;

        float r_ent = maze_scale / 2;


        int agent_cell = options.get<int32_t>("agent_cell");
        int diamond_cell = options.get<int32_t>("diamond_cell");


        maze_gen = std::make_shared<MazeGen>(&rand_gen, maze_dim);
        maze_gen->generate_maze_with_doors(num_keys);
        // maze_gen->generate_maze(agent_cell,diamond_cell);
        // maze_gen->generate_maze();


        // move agent out of the way for maze generation
        agent->x = -1;
        agent->y = -1;

        int off_x = rand_gen.randn(world_dim - maze_dim + 1);
        int off_y = rand_gen.randn(world_dim - maze_dim + 1);

        for (int i = 0; i < grid_size; i++) {
          set_obj(i, WALL_OBJ);
        }

        std::pair<float, float> agent_pos(-1.0,-1.0);
        std::pair<float, float> diamond_pos(-1.0,-1.0);

        int num_placed_doors = 0;
        for (int i = 0; i < maze_dim; i++) {
            for (int j = 0; j < maze_dim; j++) {
                int x = off_x + i;
                int y = off_y + j;

                int obj = maze_gen->grid.get(i + MAZE_OFFSET, j + MAZE_OFFSET);

                float obj_x = (x + .5) * maze_scale;
                float obj_y = (y + .5) * maze_scale;

                if (obj != WALL_OBJ) {
                    set_obj(x, y, SPACE);
                }else{
                  float chance = rand_gen.rand01();
                  if (chance < wall_chance){
                    set_obj(x, y, WALL_OBJ);
                  }else if (chance < water_chance){
                    set_obj(x, y, SPACE);
                    auto ent = add_entity(x + .5, y + .5, 0, 0, .5, WATER);
                  }else if (chance < fire_chance){
                    set_obj(x, y, SPACE);
                    auto ent = add_entity(x + .5, y + .5, 0, 0
                      , .5, FIRE);
                  }else{
                    set_obj(x, y, SPACE);
                  }
                }

                if (obj >= KEY_OBJ) {
                    auto ent = spawn_entity(.375 * maze_scale, KEY, maze_scale * x, maze_scale * y, maze_scale, maze_scale);
                    ent->image_theme = obj - KEY_OBJ - 1;
                    match_aspect_ratio(ent);
                } else if (obj >= DOOR_OBJ) {
                    if (num_placed_doors < num_doors){
                      //--- UGLY HACK BUT CURRENTLY REQUIRED FOR grid_step
                      if (grid_step){
                        set_obj(x, y, LOCKED_DOOR);
                      }
                      //---

                      auto ent = add_entity(obj_x, obj_y, 0, 0, r_ent, LOCKED_DOOR);
                      ent->image_theme = obj - DOOR_OBJ - 1;
                      num_placed_doors++;
                    }
                } else if (obj == EXIT_OBJ) {
                    diamond_pos.first = x+0.5;
                    diamond_pos.second = y+0.5;
                } else if (obj == AGENT_OBJ) {
                  agent_pos.first = obj_x;
                  agent_pos.second = obj_y;
                }
            }
        }


        auto diamond = spawn_entity(.375 * maze_scale, EXIT, diamond_pos.first, diamond_pos.second, maze_scale, maze_scale);
        diamond->x = -1;
        diamond->y = -1;

        match_aspect_ratio(diamond);

        auto cells = rand_gen.simple_choose(grid_size, grid_size);
        std::vector<std::pair<int,int>> agent_diamond_pairs;
        uint32_t max_agent_diamond_locations = 5;
        int _curr_agent_cell = -1;


        for (int cell : cells){
          if(get_obj(cell) != SPACE){
            continue;
          }
          if(_curr_agent_cell == -1){
            agent->x = (cell % main_width) + .5;
            agent->y = (cell / main_width) + .5;
            if(has_any_collision(agent,0,true)){
              continue;
            }
            _curr_agent_cell = cell;
          }else{
            diamond->x = (cell % main_width) + .5;
            diamond->y = (cell / main_width) + .5;
            if(has_any_collision(diamond,0,true)){
              continue;
            }
            if(sqrt(pow(diamond->x-agent->x,2)+pow(diamond->y-agent->y,2)) <= 2.3){
              continue;
            }
            agent_diamond_pairs.push_back(std::pair<int,int>(_curr_agent_cell, cell));
            _curr_agent_cell = -1;
          }

          if(agent_diamond_pairs.size() >= max_agent_diamond_locations){
            break;
          }
        }

        bool agent_is_first = placement_rand_gen.randbool();
        int agent_diamond_rdx  = placement_rand_gen.randn(agent_diamond_pairs.size());

        if (agent_cell == -1){
          agent->x = (agent_diamond_pairs[0].first % main_width) + .5;
          agent->y = (agent_diamond_pairs[0].first / main_width) + .5;
        }else if (agent_cell == -2){
          if(agent_is_first){
            agent->x = (agent_diamond_pairs[agent_diamond_rdx].first % main_width) + .5;
            agent->y = (agent_diamond_pairs[agent_diamond_rdx].first / main_width) + .5;
          }else{
            agent->x = (agent_diamond_pairs[agent_diamond_rdx].second % main_width) + .5;
            agent->y = (agent_diamond_pairs[agent_diamond_rdx].second / main_width) + .5;
          }
        }else{
          fassert(agent_cell >= 0);
          fassert(agent_cell < grid_size);

          fassert(get_obj(agent_cell) == SPACE);

          agent->x = (agent_cell % main_width) + .5;
          agent->y = (agent_cell / main_width) + .5;

          fassert(!has_any_collision(agent,0,true));
        }

        if (diamond_cell == -1){
          agent->x = (agent_diamond_pairs[0].second % main_width) + .5;
          agent->y = (agent_diamond_pairs[0].second / main_width) + .5;
        }else if (diamond_cell == -2){
          if(agent_is_first){
            diamond->x = (agent_diamond_pairs[agent_diamond_rdx].second % main_width) + .5;
            diamond->y = (agent_diamond_pairs[agent_diamond_rdx].second / main_width) + .5;
          }else{
            diamond->x = (agent_diamond_pairs[agent_diamond_rdx].first % main_width) + .5;
            diamond->y = (agent_diamond_pairs[agent_diamond_rdx].first / main_width) + .5;
          }
        }else{
          fassert(diamond_cell >= 0);
          fassert(diamond_cell < grid_size);

          fassert(get_obj(diamond_cell) == SPACE);

          diamond->x = (diamond_cell % main_width) + .5;
          diamond->y = (diamond_cell / main_width) + .5;

          fassert(!has_any_collision(diamond,0,true));
        }



        float ring_key_r = 0.03f;

        for (int i = 0; i < num_keys; i++) {
            auto ent = add_entity(1 - ring_key_r * (2 * i + 1.25), ring_key_r * .75, 0, 0, ring_key_r, KEY_ON_RING);
            ent->image_theme = i;
            ent->rotation = PI / 2;
            ent->render_z = 1;
            ent->use_abs_coords = true;
            match_aspect_ratio(ent);
        }

        auto ptr_state_obs = point_to_obs<uint8_t>("state");
        std::cout << "should be writing " << ptr_state_obs << std::endl;
        if (ptr_state_obs != 0){
          std::cout << "am writing" << std::endl;
          write_state_to_buffer(ptr_state_obs);
        }
    }


    void write_state_to_buffer(uint8_t *ptr){
      ptr[0] = int(agent->y) * main_width + int(agent->x);
      for (int i =0; i < 3; i++){
        ptr[i+1] = has_keys[i];
      }

      for (int i =0; i < 3; i++){
        ptr[i+4] = 0;
      }

      for (int i = 0; i < grid_size; i++) {
        ptr[i+7] = asset_to_state.at(get_obj(i));
      }

      uint8_t asset_subset = 0;
      for (auto entity : entities){
        asset_subset = 0;
        if (entity->type == PLAYER){
          continue;
        }else if (entity->type == KEY_ON_RING){
          continue;
        }else if (entity->type == LOCKED_DOOR){
          asset_subset = entity->image_theme;
          ptr[4+asset_subset] = 1;
        }else if (entity->type == KEY){
          asset_subset = entity->image_theme;
        }
        auto i = int(entity->y) * main_width + int(entity->x);
        ptr[i+7] = asset_to_state.at(entity->type+asset_subset);
      }
    }

    void game_step() override {
        BasicAbstractGame::game_step();

        agent->face_direction(action_vx, action_vy);

        step_data.reward += action_bonus;

        auto ptr_state_info = point_to_info<uint8_t>("state");
        if (ptr_state_info != 0){
          write_state_to_buffer(ptr_state_info);
        }

        auto ptr_state_obs = point_to_obs<uint8_t>("state");
        if (ptr_state_obs != 0){
          write_state_to_buffer(ptr_state_obs);
        }

    }


};

REGISTER_GAME("heistpp", HeistPPGame);
