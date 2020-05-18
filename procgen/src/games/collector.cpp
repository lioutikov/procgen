#include "../basic-abstract-game.h"
#include "../assetgen.h"
#include "../roomgen.h"
#include <set>
#include <queue>
#include <QPainterPath>

const float GOAL_REWARD = 10.0f;
const float TARGET_REWARD = 3.0f;

const int GOAL = 1;
const int OBSTACLE = 2;
const int TARGET = 3;
const int PLAYER_BULLET = 4;
const int ENEMY = 5;
const int CAVEWALL = 8;
const int EXHAUST = 9;
const int GOAL_RED = 10;
const int GOAL_GREEN = 11;
const int RESOURCE_RED = 12;
const int RESOURCE_GREEN = 13;
const int FUEL= 14;

const int MARKER = 1003;


class Collector : public BasicAbstractGame {

  class CellManager {

  protected:
    std::vector<int> cells;
    RandGen* rand_gen;

    int main_width;
    int main_height;

  public:

    CellManager(RandGen* _rand_gen): rand_gen(_rand_gen), main_width(100), main_height(100){
    }

    void filter(bool (*func)(int cell)){
      std::vector<int> tmp;
      for (int cell : cells){
        if (func(cell)){
          tmp.push_back(cell);
        }
      }
      cells = tmp;
    }

    int get_world_dim(){
      return main_width-6;
    }

    void randomize(){
      cells = rand_gen->choose_n(cells, (int)(cells.size()));
    }

    int pop_at(int idx){
      int cell = cells[idx];
      cells.erase(cells.begin()+idx);
      return cell;
    }

    int pop_cell(int cell){
      int idx = index(cell);
      return pop_at(idx);
    }

    int pop_next (){
      return pop_at(0);
    }

    int pop_random(){
      int idx = rand_gen->randn((int)(cells.size()));
      return pop_at(idx);
    }

    int pop_random_min_distance_from(std::pair<float,float> min_center, float min_dist){
      int rdx = rand_gen->randn((int)(cells.size()));
      for (uint32_t i = 0; i < cells.size(); i++){
        int idx = (i+rdx) % cells.size();
        int cell = cells[idx];
        std::pair<float,float> pos = cell_to_pos(cell);

        float dist = sqrt(pow(pos.first-min_center.first,2)+pow(pos.second-min_center.second,2));
        if (dist >= min_dist){
          return pop_at(idx);
        }
      }
    }

    int pop_random_max_distance_from(std::pair<float,float> max_center, float max_dist){
      int rdx = rand_gen->randn((int)(cells.size()));
      for (uint32_t i = 0; i < cells.size(); i++){
        int idx = (i+rdx) % cells.size();
        int cell = cells[idx];
        std::pair<float,float> pos = cell_to_pos(cell);

        float dist = sqrt(pow(pos.first-max_center.first,2)+pow(pos.second-max_center.second,2));
        if (dist <= max_dist){
          return pop_at(idx);
        }
      }
    }

    int pop_random_min_max_distance_from(std::pair<float,float> min_center, float min_dist, std::pair<float,float> max_center, float max_dist){
      int rdx = rand_gen->randn((int)(cells.size()));
      for (uint32_t i = 0; i < cells.size(); i++){
        int idx = (i+rdx) % cells.size();
        int cell = cells[idx];
        std::pair<float,float> pos = cell_to_pos(cell);

        float dist_a = sqrt(pow(pos.first-min_center.first,2)+pow(pos.second-min_center.second,2));
        float dist_b = sqrt(pow(pos.first-max_center.first,2)+pow(pos.second-max_center.second,2));
        if ((dist_a >= min_dist) && (dist_b <= max_dist)){
          return pop_at(idx);
        }
      }
      return -1;
    }

    int pop_random_min_max_distance_from(std::vector<std::shared_ptr<Entity> > *entities, float min_dist, std::pair<float,float> max_center, float max_dist){
      int rdx = rand_gen->randn((int)(cells.size()));
      for (uint32_t i = 0; i < cells.size(); i++){
        int idx = (i+rdx) % cells.size();
        int cell = cells[idx];
        std::pair<float,float> pos = cell_to_pos(cell);

        if (min_dist > 0.0){
          bool valid = true;
          for (auto ent_it = entities->begin(); ent_it != entities->end(); ++ent_it){
            if( pow(pos.first-(*ent_it)->x,2)+ pow(pos.second-(*ent_it)->y,2) < pow(min_dist,2)){
              valid = false;
              break;
            }
          }
          if (!valid) continue;
        }

        if (max_dist > 0.0){
          if (pow(pos.first-max_center.first,2)+ pow(pos.second-max_center.second,2) > pow(max_dist,2)) continue;
        }
        return pop_at(idx);
      }
    }

    int pop_random_min_min_max_distance_from(std::pair<float,float> min_center0, float min_dist0, std::vector<std::shared_ptr<Entity> > *entities, float min_dist, std::pair<float,float> max_center, float max_dist){

      int rdx = rand_gen->randn((int)(cells.size()));
      for (uint32_t i = 0; i < cells.size(); i++){
        int idx = (i+rdx) % cells.size();
        int cell = cells[idx];
        std::pair<float,float> pos = cell_to_pos(cell);

        if (min_dist0 > 0.0){
          if (pow(pos.first-min_center0.first,2)+ pow(pos.second-min_center0.second,2) < pow(min_dist0,2)){
            continue;
          }
        }

        if (max_dist > 0.0){
          if (pow(pos.first-max_center.first,2)+ pow(pos.second-max_center.second,2) > pow(max_dist,2)) continue;
        }

        if (min_dist > 0.0){
          bool valid = true;
          for (auto ent_it = entities->begin(); ent_it != entities->end(); ++ent_it){
            if( pow(pos.first-(*ent_it)->x,2)+ pow(pos.second-(*ent_it)->y,2) < pow(min_dist,2)){
              valid = false;
              break;
            }
          }
          if (!valid) continue;
        }

        return pop_at(idx);
      }

      return -1;
    }

    int pop_random_in_line(std::pair<float,float> pos0, std::pair<float,float> pos1, std::vector<std::shared_ptr<Entity> > *entities, float min_dist, std::pair<float,float> max_center, float max_dist){

      std::vector<int> shifts;

      for(int n = 1; n < fabs(pos1.first-pos0.first); n++){
        shifts.push_back(n);
      }

      for(int n = 1; n < fabs(pos1.second-pos0.second); n++){
        shifts.push_back(-n);
      }

      std::set<int> skip;
      for (int n : rand_gen->choose_n(shifts,shifts.size())){
        std::pair<float,float> base = pos0;
        if (n >= 0){ //try shift in x
          if (pos1.first < pos0.first){
            n = -n;
          }
          base.first += n;
          base.second += (pos1.second-pos0.second)/(pos1.first-pos0.first)*n;
        }else{ //try shift in y
          if (pos1.second > pos0.second){
            n = -n;
          }
          base.first += (pos1.first-pos0.first)/(pos1.second-pos0.second)*n;
          base.second += n;
        }

        // random offsets
        for(int roff : rand_gen->simple_choose(9,9)){
          int xoff = (roff % 3) -1;
          int yoff = (roff / 3) -1;

          std::pair<float,float> candidate(base.first+xoff,base.second+yoff);

          int cell = pos_to_cell(candidate);
          if (skip.find(cell) != skip.end()) continue;

          if (!contains(cell)){
            skip.insert(cell);
            continue;
          }

          if (min_dist > 0.0){
            bool valid = true;
            for (auto ent_it = entities->begin(); ent_it != entities->end(); ++ent_it){
              if( pow(candidate.first-(*ent_it)->x,2)+ pow(candidate.second-(*ent_it)->y,2) < pow(min_dist,2)){
                valid = false;
                skip.insert(cell);
                break;
              }
            }
            if (!valid) continue;
          }

          if (max_dist > 0.0){
            if (pow(candidate.first-max_center.first,2)+ pow(candidate.second-max_center.second,2) > pow(max_dist,2)){
              skip.insert(cell);
              continue;
            }
          }
            return pop_cell(cell);
          }

      }
      return -1;
    }


    void push(int cell){
      if (!contains(cell)){
        cells.push_back(cell);
      }
    }

    bool contains(int cell){
      std::vector<int>::iterator it = std::find(cells.begin(), cells.end(), cell);
      return (it != cells.end());
    }

    int peek(int idx){
      return cells[idx];
    }

    int peek_next(){
      return peek(0);
    }

    int index(int cell){
      std::vector<int>::iterator it = std::find(cells.begin(), cells.end(), cell);
      return std::distance(cells.begin(), it);
    }

    int size(){
      return cells.size();
    }

    void add_cells(std::vector<int>::const_iterator begin, std::vector<int>::const_iterator end){
      cells.insert(cells.end(), begin, end);
    }

    std::pair<float,float> cell_to_pos(int cell){
      return std::pair<float,float>((cell % main_width) + .5, (cell / main_width) + .5);
    }

    int pos_to_cell(float x, float y){
      return int(y) * main_width + int(x);
    }

    int pos_to_cell(std::pair<float,float> pos){
      return pos_to_cell(pos.first, pos.second);
    }

    std::pair<float,float> get_center(){
      return std::pair<float,float>((main_width-5.0-1.0)/2.0 + (5.0+1.0)/2.0, (main_height-5.0-1.0)/2.0 + 1.0);
    }

    std::pair<float,float> peek_mirrored_point(const std::pair<float,float> &src, const std::pair<float,float> &mirror_point){
      return std::pair<float,float> (float(int(2.0*mirror_point.first-src.first))+0.5,float(int(2.0*mirror_point.second-src.second))+0.5);
    }

    std::pair<float,float> pop_mirrored_point(const std::pair<float,float> &src, const std::pair<float,float> &mirror_point){
      std::pair<float,float> target = peek_mirrored_point(src,mirror_point);
      pop_cell(pos_to_cell(target.first, target.second));
      return target;
    }

    std::pair<float,float> pop_mirrored_point(const std::pair<float,float> &src){
      return pop_mirrored_point(src, get_center());
    }

    std::pair<float,float> get_projection_on_line(const std::pair<float,float> &src, const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b){
      float a = mirror_line_b.second-mirror_line_a.second;
      float b = mirror_line_a.first-mirror_line_b.first;
      float c = mirror_line_b.first*mirror_line_a.second-mirror_line_a.first*mirror_line_b.second;
      float d = pow(a,2) + pow(b,2);
      return std::pair<float,float> ((b*(b*src.first-a*src.second)-a*c)/d,(a*(a*src.second-b*src.first)-b*c)/d);
    }

    std::pair<float,float> pop_mirrored_point(const std::pair<float,float> &src, const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b){
      return pop_mirrored_point(src, get_projection_on_line(src, mirror_line_a, mirror_line_b));
    }

    std::pair<std::pair<float,float>,std::pair<float,float>> get_mirrored_pair(const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b){
      // int rand_idx = rand_gen->randn((int)(cells.size()));
      //
      // std::pair<float,float> c1;
      // std::pair<float,float> c2;
      // for (auto i = 0; i < cells.size(); i++){
      //   int idx = (i+rand_idx) % cells.size();
      //   c1 = cell_to_pos(cells[idx]);
      //   c2 = peek_mirrored_point(c1,mirror_line_a,mirror_line_b);
      //   if (!contains(pos_to_cell(c2))) continue;
      //   return std::pair<std::pair<float,float>,std::pair<float,float>>(c1,c2);
      // }
      // std::cout << "No fitting pair found" << std::endl;
      // fassert(false);

      return get_mirrored_pair(mirror_line_a, mirror_line_b, 0.0, 0.0, NULL);

    }

    std::pair<std::pair<float,float>,std::pair<float,float>> get_mirrored_pair(const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b, float min_distance){
      // int rand_idx = rand_gen->randn((int)(cells.size()));
      //
      // std::pair<float,float> c1;
      // std::pair<float,float> c2;
      // for (auto i = 0; i < cells.size(); i++){
      //   int idx = (i+rand_idx) % cells.size();
      //   c1 = cell_to_pos(cells[idx]);
      //   c2 = peek_mirrored_point(c1,mirror_line_a,mirror_line_b);
      //   if (!contains(pos_to_cell(c2))) continue;
      //   if (min_distance > 0.0) && (pow(c1.first-c2.first,2)+ pow(c1.second-c2.second,2) < pow(min_distance,2)) continue;
      //   return std::pair<std::pair<float,float>,std::pair<float,float>>(c1,c2);
      // }
      // std::cout << "No fitting pair found" << std::endl;
      // fassert(false);

      return get_mirrored_pair(mirror_line_a, mirror_line_b, min_distance, 0.0, NULL);
    }

    std::pair<std::pair<float,float>,std::pair<float,float>> get_mirrored_pair(const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b, float min_distance, float min_radius, std::vector<std::shared_ptr<Entity> > *entities){
      // int rand_idx = rand_gen->randn((int)(cells.size()));
      //
      // int ci1,ci2;
      // std::pair<float,float> c1;
      // std::pair<float,float> c2;
      // for (uint32_t i = 0; i < cells.size(); i++){
      //   ci1 = cells[(i+rand_idx) % cells.size()];
      //   c1 = cell_to_pos(ci1);
      //   c2 = peek_mirrored_point(c1,get_projection_on_line(c1,mirror_line_a,mirror_line_b));
      //   ci2 = pos_to_cell(c2);
      //   if (ci1 == ci2) continue;
      //   if (!contains(ci2)) continue;
      //   if ((min_distance > 0.0) && (pow(c1.first-c2.first,2)+ pow(c1.second-c2.second,2) < pow(min_distance,2))) continue;
      //   if (min_radius > 0.0){
      //     bool valid = true;
      //     for (auto ent_it = entities->begin(); ent_it != entities->end(); ++ent_it){
      //       if( pow(c1.first-(*ent_it)->x,2)+ pow(c1.second-(*ent_it)->y,2) < pow(min_radius,2)){
      //         valid = false;
      //         break;
      //       }
      //       if( pow(c2.first-(*ent_it)->x,2)+ pow(c2.second-(*ent_it)->y,2) < pow(min_radius,2)){
      //         valid = false;
      //         break;
      //       }
      //     }
      //     if (!valid) continue;
      //   }
      //   // std::cout << "c1 c2: " << c1.first << "," << c1.second << " " << c2.first << "," << c2.second << std::endl;
      //   // std::cout << "ci1 ci2: " << ci1 << " " << ci2 << std::endl;
      //   pop_cell(ci1);
      //   pop_cell(ci2);
      //   return std::pair<std::pair<float,float>,std::pair<float,float>>(c1,c2);
      // }
      // std::cout << "No fitting pair found" << std::endl;
      // fassert(false);

      return get_mirrored_pair(mirror_line_a, mirror_line_b, min_distance, 0.0, entities, 0.0);
    }


    std::pair<std::pair<float,float>,std::pair<float,float>> get_mirrored_pair(const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b, float min_distance, float min_radius, std::vector<std::shared_ptr<Entity> > *entities, float max_off_center){
      int rand_idx = rand_gen->randn((int)(cells.size()));

      int ci1,ci2;
      std::pair<float,float> c1;
      std::pair<float,float> c2;
      for (uint32_t i = 0; i < cells.size(); i++){
        ci1 = cells[(i+rand_idx) % cells.size()];
        c1 = cell_to_pos(ci1);
        c2 = peek_mirrored_point(c1,get_projection_on_line(c1,mirror_line_a,mirror_line_b));
        ci2 = pos_to_cell(c2);
        if (ci1 == ci2) continue;
        if (!contains(ci2)) continue;
        if ((min_distance > 0.0) && (pow(c1.first-c2.first,2)+ pow(c1.second-c2.second,2) < pow(min_distance,2))) continue;
        if (max_off_center > 0.0){
          auto cter = get_center();
          if (pow(c1.first-cter.first,2)+ pow(c1.second-cter.second,2) > pow(max_off_center,2)) continue;
          if (pow(c2.first-cter.first,2)+ pow(c2.second-cter.second,2) > pow(max_off_center,2)) continue;
        }
        if (min_radius > 0.0){
          bool valid = true;
          for (auto ent_it = entities->begin(); ent_it != entities->end(); ++ent_it){
            if( pow(c1.first-(*ent_it)->x,2)+ pow(c1.second-(*ent_it)->y,2) < pow(min_radius,2)){
              valid = false;
              break;
            }
            if( pow(c2.first-(*ent_it)->x,2)+ pow(c2.second-(*ent_it)->y,2) < pow(min_radius,2)){
              valid = false;
              break;
            }
          }
          if (!valid) continue;
        }
        // std::cout << "c1 c2: " << c1.first << "," << c1.second << " " << c2.first << "," << c2.second << std::endl;
        // std::cout << "ci1 ci2: " << ci1 << " " << ci2 << std::endl;
        pop_cell(ci1);
        pop_cell(ci2);
        return std::pair<std::pair<float,float>,std::pair<float,float>>(c1,c2);
      }
      std::cout << "No fitting pair found" << std::endl;
      fassert(false);
    }




    void reset(int width, int height){
      main_width = width;
      main_height = height;
      cells.clear();
    }

  };


    class Resource : public Entity {

      protected:

        float value;
        float init_value;
        float step_value;
        float max_value;
        bool existing;

        // std::make_shared<Entity>(src->x, src->y, vx, vy, obj_r, type);

      public:
        Resource(float _init_value, float _step_value, float _max_value, float _x, float _y, float _vx, float _vy, float _r, int _type): Entity(_x, _y, _vx, _vy, _r, _type), value(_init_value), init_value(_init_value), step_value(_step_value), max_value(_max_value){
        }

        void step() override {
          Entity::step();
          value += std::min(step_value, max_value-value);
        }

        float deposit(float net_value) {
          net_value = fmin(net_value, max_value-value);
          value += net_value;
          return net_value;
        }

        float withdraw(float net_value) {
          net_value = fmin(value, net_value);
          value -= net_value;
          return net_value;
        }

        float get_value(){
          return value;
        }

        float get_max(){
          return max_value;
        }

        float get_percentage(){
          return value/max_value;
        }

        void disappear(){
          existing = false;
          collides_with_entities = false;
          will_erase = true;
        }

        void appear(){
          appear(x,y);
        }

        void appear(float new_x, float new_y){
          will_erase = false;
          x = new_x;
          y = new_y;
          collides_with_entities = true;
          existing = true;
        }

        void push_state_to_vector(std::vector<float> &state){
          if (type == RESOURCE_RED){
            state.push_back(1.0);
          }else if (type == RESOURCE_GREEN){
            state.push_back(2.0);
          }else if (type == FUEL){
            state.push_back(3.0);
          }
          state.push_back(x);
          state.push_back(y);
          state.push_back(existing);
        }

    };


  class ResourceContainer {

  protected:
    float max_value;
    float value;

  public:

    ResourceContainer(float _max_value, float _value): max_value(_max_value),value(_value){
    }

    float deposit(float net_value) {
      // std::cout << "depositfun " << net_value << " " << max_value << " " << value << " " << max_value-value <<std::endl;
      net_value = fmin(net_value, max_value-value);
      // std::cout << "new_net_value" << net_value <<std::endl;
      value += net_value;
      return net_value;
    }

    float withdraw(float net_value) {
      net_value = fmin(value, net_value);
      value -= net_value;
      return net_value;
    }

    float get_value(){
      return value;
    }

    float get_max(){
      return max_value;
    }

    float get_percentage(){
      return value/max_value;
    }

    float consume(const std::shared_ptr<Resource> &resource){
      return resource->withdraw(deposit(resource->get_value()));
    }

    float consume_greedy(const std::shared_ptr<Resource> &resource){
      float consumed_value = resource->withdraw(resource->get_value());
      return deposit(consumed_value);
    }

    void reset(){
      value = 0.0;
    }

    void reset(float net_value){
      net_value = fmin(net_value, max_value);
      value = net_value;
    }

  };

  class ResourceContainerSlotted : public ResourceContainer {

  protected:
    std::vector<std::pair<int,float>> resource_slots;

  public:

    ResourceContainerSlotted(float _max_value, float _value): ResourceContainer(_max_value, _value){
    }

    float deposit(float net_value, int type, bool same_slot) {
      net_value = ResourceContainer::deposit(net_value);
      if (net_value > 0.0){
        if (resource_slots.empty() || !same_slot || (resource_slots.back().first != type)){
          resource_slots.push_back(std::pair<int,float>(type,net_value));
        }else{
          resource_slots.back().second += net_value;
        }
      }
      return net_value;
    }

    float deposit(float net_value, int type){
      return deposit(net_value, type, false);
    }

    float withdraw(float net_value) {
      throw std::logic_error("Can not withdraw float value from ResourceContainerSlotted");
    }

    std::pair<int,float> withdraw_last_slot(){
      if (resource_slots.empty()){
        return std::pair<int,float>(0,0.0);
      }
      std::pair<int,float> res = resource_slots.back();
      resource_slots.pop_back();
      float net_value = ResourceContainer::withdraw(res.second);
      if (net_value != res.second){
        std::cout << "net_value: " << net_value << " res.second: " << res.second << std::endl;
        throw std::logic_error("Something went very wrong here...");
      }
      return res;
    }

    int num_slots(){
      return resource_slots.size();
    }

    int get_resource_type_at_slot(int idx){
      return resource_slots[idx].first;
    }

    int get_value_at_slot(int idx){
      return resource_slots[idx].second;
    }

    std::pair<int,float> withdraw_slot(int idx){
      if (resource_slots.empty()){
        return std::pair<int,float>(0,0.0);
      }
      std::pair<int,float> res = resource_slots[idx];
      resource_slots.erase(resource_slots.begin()+idx);
      float net_value = ResourceContainer::withdraw(res.second);
      if (net_value != res.second){
        std::cout << "net_value: " << net_value << " res.second: " << res.second << std::endl;
        throw std::logic_error("Something went very wrong here...");
      }
      return res;
    }

    std::vector<std::pair<int,float>> get_percentages(){
      std::vector<std::pair<int,float>> res;
      for (auto p : resource_slots ){
        res.push_back(std::pair<int,float>(p.first,p.second/max_value));
      }
      return res;
    }

    float consume(const std::shared_ptr<Resource> &resource){
      return resource->withdraw(deposit(resource->get_value(), resource->type));
    }

    float get_value(int type){
      float res_value = 0.0;
      for (auto p : resource_slots){
        if (p.first == type){
          res_value += p.second;
        }
      }
      return res_value;
    }

    float get_value(){
      return ResourceContainer::get_value();
    }

    void reset(){
      resource_slots.clear();
      ResourceContainer::reset();
    }
  };

  class Goal : public Entity {

    protected:

      float value;
      float init_value;
      float step_value;
      float max_value;
      bool existing;
      std::vector<int> accepted_resource_types;
      std::vector<int> ignored_resource_types;

    public:
      // std::make_shared<Entity>(src->x, src->y, vx, vy, obj_r, type);

      Goal(float _init_value, float _step_value, float _max_value, float _x, float _y, float _vx, float _vy, float _r, int _type): Entity(_x, _y, _vx, _vy, _r, _type), value(_init_value), init_value(_init_value), step_value(_step_value), max_value(_max_value){
      }

      void add_resource_type(int resource_type){
        accepted_resource_types.push_back(resource_type);
      }

      void add_ignore_type(int resource_type){
        ignored_resource_types.push_back(resource_type);
      }

      bool accepts_resource_type(int resource_type){
        std::vector<int>::iterator it = std::find(accepted_resource_types.begin(), accepted_resource_types.end(), resource_type);
        return (it != accepted_resource_types.end());
      }

      bool ignores_resource_type(int resource_type){
        std::vector<int>::iterator it = std::find(ignored_resource_types.begin(), ignored_resource_types.end(), resource_type);
        return (it != ignored_resource_types.end());
      }

      void step() override {
        Entity::step();
        value += fmin(step_value, max_value-value);
      }

      float get_value(){
        return value;
      }

      float get_max(){
        return max_value;
      }

      float get_percentage(){
        return value/max_value;
      }

      float deposit(float net_value) {
        net_value = fmin(net_value, max_value-value);
        value += net_value;
        return net_value;
      }

      float consume(const std::shared_ptr<ResourceContainer> &container, int resource_type){
        float net_value = container->withdraw(container->get_value());
        if (! accepts_resource_type(resource_type)){
          return 0.0;
        }
        return deposit(net_value);
      }

      float consume(const std::shared_ptr<ResourceContainerSlotted> &container){
        float net_value = 0.0;

        for (int idx = container->num_slots()-1; idx >=0; idx--){
          if(ignores_resource_type(container->get_resource_type_at_slot(idx))){
            continue;
          }
          std::pair<int,float> slot = container->withdraw_slot(idx);
          if (accepts_resource_type(slot.first)){
            net_value += deposit(slot.second);
          }
        }
        return net_value;
        //
        //
        // while (container->get_value() > 0.0){
        //   std::pair<int,float> slot = container->withdraw_last_slot();
        //
        //   if (accepts_resource_type(slot.first)){
        //     net_value += deposit(slot.second);
        //   }
        // }
        // return net_value;
      }

      void reset(){
        value = 0.0;
      }

      void push_state_to_vector(std::vector<float> &state){
        if (type == GOAL_RED){
          state.push_back(1.0);
        }else if (type == GOAL_GREEN){
          state.push_back(2.0);
        }
        state.push_back(x);
        state.push_back(y);
        state.push_back(get_value());
      }

  };

  class GoalManager{

  protected:
    std::vector<std::shared_ptr<Goal>> goals;
    std::vector<std::shared_ptr<Entity> > *entities;
    float goal_init;
    float goal_step;
    float goal_max;

  public:
    GoalManager(std::vector<std::shared_ptr<Entity> > *_entities, float _goal_init, float _goal_step, float _goal_max): entities(_entities), goal_init(_goal_init), goal_step(_goal_step), goal_max(_goal_max){

    }

    std::shared_ptr<Goal> spawn(int goal_type, std::pair<float,float> pos){
      auto goal = std::make_shared<Goal>(goal_init, goal_step, goal_max, pos.first, pos.second, 0.0, 0.0, 0.5, goal_type);
      goal->collides_with_entities = true;
      if (goal_type == GOAL_GREEN){
        goal->add_resource_type(RESOURCE_GREEN);
      }else if (goal_type == GOAL_RED){
        goal->add_resource_type(RESOURCE_RED);
      }else{
        std::cout << "UNKNOWN GOAL TYPE: " << goal_type << std::endl;
        fassert(false);
      }
      goals.push_back(goal);
      entities->push_back(goal);
      return goal;
    }

    std::vector<std::shared_ptr<Goal>> get_goals(int goal_type){
      std::vector<std::shared_ptr<Goal>> res;
      for (auto goal : goals){
        if (goal->type == goal_type){
          res.push_back(goal);
        }
      }
      return res;
    }

    void reset(){
      goals.clear();
    }

    void push_state_to_vector(std::vector<float> &state){
      for (auto goal : goals){
        goal->push_state_to_vector(state);
      }
    }
  };

  class ResourceRespawnTime{

  public:
    virtual int operator () ()=0;

    virtual ~ResourceRespawnTime(){

    }

  };

  class ResourceRespawnTimeNow : public ResourceRespawnTime{

  public:

    virtual int operator () () override {
      return 0;
    }

  };

  class ResourceRespawnTimeFix : public ResourceRespawnTime{

  protected:
    int delay;

  public:
    ResourceRespawnTimeFix(int _delay):  ResourceRespawnTime(), delay(_delay){
    }

    virtual int operator () () override {
      return delay;
    }

  };

  class ResourceRespawnTimeRandom : public ResourceRespawnTime{

  protected:
    RandGen* rand_gen;
    int lower;
    int upper;

  public:
    ResourceRespawnTimeRandom(RandGen* _rand_gen, int _lower, int _upper):  ResourceRespawnTime(), rand_gen(_rand_gen), lower(_lower), upper(_upper){
    }

    virtual int operator () () override {
      return rand_gen->randint(lower, upper);
    }
  };

  class ResourceRespawnLocation {

protected:
    std::shared_ptr<CellManager> free_cells;

public:
    ResourceRespawnLocation(std::shared_ptr<CellManager> &_free_cells): free_cells(_free_cells){
    }

    virtual int operator () (int last_cell, float agent_x, float agent_y)= 0;

    virtual ~ResourceRespawnLocation(){

    }

  };

  class ResourceRespawnLocationStatic : public ResourceRespawnLocation {

    public:
    ResourceRespawnLocationStatic(std::shared_ptr<CellManager> &_free_cells):ResourceRespawnLocation(_free_cells){
    }

    virtual int operator () (int last_cell, float agent_x, float agent_y){
      return last_cell;
    }

  };

  class ResourceRespawnLocationNext : public ResourceRespawnLocation {

  public:
    ResourceRespawnLocationNext(std::shared_ptr<CellManager> &_free_cells):ResourceRespawnLocation(_free_cells){
    }

    virtual int operator () (int last_cell, float agent_x, float agent_y){
      free_cells->push(last_cell);
      return free_cells->pop_next();
    }

  };

  class ResourceRespawnLocationRandom : public ResourceRespawnLocation {

  public:
    ResourceRespawnLocationRandom(std::shared_ptr<CellManager> &_free_cells):ResourceRespawnLocation(_free_cells){
    }

    virtual int operator () (int last_cell, float agent_x, float agent_y){
      free_cells->push(last_cell);
      return free_cells->pop_random();
    }

  };

  class ResourceRespawnLocationNextAway : public ResourceRespawnLocation {

  protected:
    float min_distance;

    public:
    ResourceRespawnLocationNextAway(std::shared_ptr<CellManager> &_free_cells, float _min_distance):ResourceRespawnLocation(_free_cells), min_distance(_min_distance){
    }

    virtual int operator () (int last_cell, float agent_x, float agent_y){
      free_cells->push(last_cell);

      std::pair<float,float> pos;
      for (int idx=0; idx < free_cells->size(); idx++){
        pos = free_cells->cell_to_pos(free_cells->peek(idx));
        if (sqrt(pow(pos.first-agent_x,2)+pow(pos.second-agent_y,2)) > min_distance){
          return free_cells->pop_at(idx);
        }
      }

      return free_cells->pop_next();
    }

  };

  class ResourceRespawnLocationRandomAway : public ResourceRespawnLocation {

  protected:
    RandGen* rand_gen;
    float min_distance;

  public:
    ResourceRespawnLocationRandomAway(RandGen* _rand_gen, std::shared_ptr<CellManager> &_free_cells, float _min_distance):ResourceRespawnLocation(_free_cells),rand_gen(_rand_gen),min_distance(_min_distance){
    }

    virtual int operator () (int last_cell, float agent_x, float agent_y){
      free_cells->push(last_cell);

      std::pair<float,float> pos;
      std::vector<int> indices = rand_gen->simple_choose((int)(free_cells->size()), (int)(free_cells->size()));
      for (int idx : indices){
        pos = free_cells->cell_to_pos(free_cells->peek(idx));
        if (sqrt(pow(pos.first-agent_x,2)+pow(pos.second-agent_y,2)) > min_distance){
          return free_cells->pop_at(idx);
        }
      }

      return free_cells->pop_random();
    }

  };

  class ResourceManager{

  protected:

      std::vector<std::shared_ptr<Resource> > resources;
      std::vector<std::shared_ptr<Entity> > *entities;

    public:
    ResourceManager(std::vector<std::shared_ptr<Entity> > *_entities): entities(_entities){
    }

    std::shared_ptr<Resource> spawn(int resource_type, std::pair<float,float> pos){
      auto resource = std::make_shared<Resource>(0.0, 1.0, 10.0, pos.first, pos.second, 0.0, 0.0, 0.5, resource_type);
      resource->collides_with_entities = true;
      resources.push_back(resource);
      entities->push_back(resource);
      return resource;
    }

    void respawn(const std::shared_ptr<Resource> &resource){
      std::cerr << "RESPAWN ME" << std::endl;
    }

    void respawn(const std::shared_ptr<Entity> &entity){
      if ((entity->type == RESOURCE_GREEN) || (entity->type == RESOURCE_RED) || (entity->type == FUEL)){
        respawn(std::static_pointer_cast<Resource>(entity));
      }
    }

    void reset(){
      resources.clear();
    }

    void push_state_to_vector(std::vector<float> &state){
      for (auto resource : resources){
        resource->push_state_to_vector(state);
      }
    }

  };

  class Obstacle : public Entity {

  public:

    Obstacle(float _x, float _y, float _vx, float _vy, float _r, int _type): Entity(_x, _y, _vx, _vy, _r, _type){
    }

    void push_state_to_vector(std::vector<float> &state){
      state.push_back(x);
      state.push_back(y);
      state.push_back(1.0);
    }
  };

  class ObstacleManager{

  protected:

    std::vector<std::shared_ptr<Obstacle>> obstacles;
    std::vector<std::shared_ptr<Entity> > *entities;

  public:
    ObstacleManager(std::vector<std::shared_ptr<Entity> > *_entities): entities(_entities){

    }

    std::shared_ptr<Obstacle> spawn(std::pair<float,float> pos){

      auto obstacle = std::make_shared<Obstacle>(pos.first, pos.second, 0.0, 0.0, 0.5, OBSTACLE);
      obstacle->collides_with_entities = true;
      obstacles.push_back(obstacle);
      entities->push_back(obstacle);
      return obstacle;
    }

    void reset(){
      obstacles.clear();
    }

    void push_state_to_vector(std::vector<float> &state){
      for (auto obstacle : obstacles){
        obstacle->push_state_to_vector(state);
      }
    }
  };


  class Ship{

  protected:
    float max_fuel;
    float init_fuel;
    float max_resources;
    float init_resources_green;
    float init_resources_red;

  public:

    std::shared_ptr<Entity> agent;
    std::shared_ptr<ResourceContainer> fuel;
    std::shared_ptr<ResourceContainerSlotted> resources;

    Ship(GameOptions &options) {

      max_fuel = options.get<float>("agent_max_fuel");
      init_fuel = options.get<float>("agent_init_fuel");
      max_resources = options.get<float>("agent_max_resources");
      init_resources_green = options.get<float>("agent_init_resources_green");
      init_resources_red = options.get<float>("agent_init_resources_red");

      fassert(max_fuel >= init_fuel);
      fuel = std::make_shared<ResourceContainer>(max_fuel, init_fuel);

      fassert(max_resources >= init_resources_green + init_resources_red);
      resources = std::make_shared<ResourceContainerSlotted>(max_resources, 0.0);

    }

    void reset(std::shared_ptr<Entity> &_agent){
      agent = _agent;
      agent->vx = 0.0;
      agent->vy = 0.0;
      agent->vrot = 0.0;

      fuel->reset(init_fuel);
      resources->reset();
      resources->deposit(init_resources_green, RESOURCE_GREEN);
      resources->deposit(init_resources_red, RESOURCE_RED);
    }

    void set_pose(std::pair<float,float> pos, std::pair<float, float> abs_direction){
      set_pose(pos);
      agent->face_direction(abs_direction.first-agent->x, abs_direction.second-agent->y,PI/2.0);
    }

    void set_pose(std::pair<float,float> pos){
      agent->x = pos.first;
      agent->y = pos.second;
    }

    void push_state_to_vector(std::vector<float> &state){
      state.push_back(agent->x);
      state.push_back(agent->y);
      state.push_back(agent->rotation);
      state.push_back(agent->vx);
      state.push_back(agent->vy);
      state.push_back(agent->vrot);
      state.push_back(fuel->get_value());
      state.push_back(resources->get_value(RESOURCE_GREEN));
      state.push_back(resources->get_value(RESOURCE_RED));
    }

  };

  struct Managers{
    std::shared_ptr<CellManager> cell;
    std::shared_ptr<ResourceManager> resource;
    std::shared_ptr<GoalManager> goal;
    std::shared_ptr<ObstacleManager> obstacle;
  };


  class InitLocator {

  protected:
    const Managers &managers;
    std::vector<std::shared_ptr<Entity> > *entities;
public:
    InitLocator(const Managers &_managers, std::vector<std::shared_ptr<Entity> > *_entities): managers(_managers), entities(_entities){
    }

    virtual void init(const std::shared_ptr<Ship> &ship, GameOptions &options) = 0;


    virtual ~InitLocator(){

    }

  };

  class InitLocatorRandom : public InitLocator {

  public:
    InitLocatorRandom(const Managers &_managers, std::vector<std::shared_ptr<Entity> > *_entities) : InitLocator(_managers, _entities){
    }


    // virtual void init(const std::shared_ptr<Entity> &agent, const std::vector<std::pair<int,int> > &spawn_list) override{
    virtual void init(const std::shared_ptr<Ship> &ship, GameOptions &options) override{


      std::pair<float,float> agent_pos = managers.cell->cell_to_pos(managers.cell->pop_random());
      std::pair<float,float> center_pos = managers.cell->get_center();
      ship->set_pose(agent_pos, center_pos);

      int num_resources_green = options.get<int32_t>("num_resources_green");
      int num_resources_red = options.get<int32_t>("num_resources_red");
      int num_fuel = options.get<int32_t>("num_fuel");
      int num_obstacles = options.get<int32_t>("num_obstacles");
      int num_goals_red = options.get<int32_t>("num_goals_red");
      int num_goals_green = options.get<int32_t>("num_goals_green");


      for (int i=0; i< num_goals_green; i++){
        managers.goal->spawn(GOAL_GREEN, managers.cell->cell_to_pos(managers.cell->pop_random()));
      }

      for (int i=0; i< num_goals_red; i++){
        managers.goal->spawn(GOAL_RED, managers.cell->cell_to_pos(managers.cell->pop_random()));
      }

      for (int i=0; i<num_resources_green; i++){
        managers.resource->spawn(RESOURCE_GREEN, managers.cell->cell_to_pos(managers.cell->pop_random()));
      }

      for (int i=0; i<num_resources_red; i++){
        managers.resource->spawn(RESOURCE_RED, managers.cell->cell_to_pos(managers.cell->pop_random()));
      }

      for (int i=0; i<num_fuel; i++){
        managers.resource->spawn(FUEL, managers.cell->cell_to_pos(managers.cell->pop_random()));
      }

      for (int i=0; i<num_obstacles; i++){
        managers.obstacle->spawn(managers.cell->cell_to_pos(managers.cell->pop_random()));
      }

    }

  };


  class InitLocatorSymmetric : public InitLocator {

  public:
    InitLocatorSymmetric(const Managers &_managers, std::vector<std::shared_ptr<Entity> > *_entities) : InitLocator(_managers, _entities){
    }

    // virtual void init(const std::shared_ptr<Entity> &agent, const std::vector<std::pair<int,int> > &spawn_list) override{
    virtual void init(const std::shared_ptr<Ship> &ship, GameOptions &options) override{


      std::pair<float,float> mirror_pos = managers.cell->get_center();
      std::pair<float,float> agent_pos = managers.cell->cell_to_pos(managers.cell->pop_random());
      ship->set_pose(agent_pos, mirror_pos);

      int num_greens = options.get<int32_t>("num_resources_green");
      int num_reds = options.get<int32_t>("num_resources_red");
      int num_fuel = options.get<int32_t>("num_fuel");
      int num_obstacles = options.get<int32_t>("num_obstacles");
      int num_red_goals = options.get<int32_t>("num_goals_red");
      int num_green_goals = options.get<int32_t>("num_goals_green");


      fassert(num_red_goals==num_green_goals);
      fassert(num_reds==num_greens);
      fassert(num_fuel % 2 == 0);
      fassert(num_obstacles % 2 == 0);

      for (auto i = 0; i < num_green_goals; i++){

        auto pair = managers.cell->get_mirrored_pair(mirror_pos, agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
        managers.goal->spawn(GOAL_GREEN, pair.first);
        managers.goal->spawn(GOAL_RED, pair.second);

      }

      for (auto i = 0; i < num_greens; i++){
        auto pair = managers.cell->get_mirrored_pair(mirror_pos, agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
        managers.resource->spawn(RESOURCE_GREEN, pair.first);
        managers.resource->spawn(RESOURCE_RED, pair.second);
      }

      for (auto i = 0; i < num_fuel/2.0; i++){
        auto pair = managers.cell->get_mirrored_pair(mirror_pos, agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
        managers.resource->spawn(FUEL, pair.first);
        managers.resource->spawn(FUEL, pair.second);
      }

      for (auto i = 0; i < num_obstacles/2.0; i++){
        auto pair = managers.cell->get_mirrored_pair(mirror_pos, agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
        managers.obstacle->spawn(pair.first);
        managers.obstacle->spawn(pair.second);
      }

    }

  };


    class InitLocatorInLine : public InitLocator {

    public:
      InitLocatorInLine(const Managers &_managers, std::vector<std::shared_ptr<Entity> > *_entities) : InitLocator(_managers, _entities){
      }

      // virtual void init(const std::shared_ptr<Entity> &agent, const std::vector<std::pair<int,int> > &spawn_list) override{
      virtual void init(const std::shared_ptr<Ship> &ship, GameOptions &options) override{



        std::pair<float,float> agent_pos = managers.cell->cell_to_pos(managers.cell->pop_random_min_max_distance_from(managers.cell->get_center(), 3.0, managers.cell->get_center(), float(managers.cell->get_world_dim())/2.0-2.0));

        int num_greens = options.get<int32_t>("num_resources_green");
        int num_reds = options.get<int32_t>("num_resources_red");
        int num_fuel = options.get<int32_t>("num_fuel");
        int num_obstacles = options.get<int32_t>("num_obstacles");
        int num_red_goals = options.get<int32_t>("num_goals_red");
        int num_green_goals = options.get<int32_t>("num_goals_green");


        fassert(num_red_goals==num_green_goals);
        fassert(num_reds==num_greens);
        fassert(num_fuel % 2 == 0);
        fassert(num_obstacles % 2 == 0);


        std::vector<std::pair<float,float>> green_goals;
        for (auto i = 0; i < num_green_goals; i++){

          auto green_goal_pos = managers.cell->cell_to_pos(managers.cell->pop_random_min_min_max_distance_from(agent_pos, 8.0, entities, 2.0,  managers.cell->get_center(), float(managers.cell->get_world_dim())/2.0 -2.0));
          auto red_goal_pos = managers.cell->cell_to_pos(managers.cell->pop_random_in_line(agent_pos, green_goal_pos, entities, 2.0,  managers.cell->get_center(), float(managers.cell->get_world_dim())/2.0 -2.0));

          auto green_goal = managers.goal->spawn(GOAL_GREEN, green_goal_pos);
          auto green_red = managers.goal->spawn(GOAL_RED, red_goal_pos);
          green_goal->add_ignore_type(RESOURCE_RED);
          green_red->add_ignore_type(RESOURCE_GREEN);
          green_goals.push_back(green_goal_pos);
        }




        for (auto i = 0; i < num_greens; i++){
          auto pair = managers.cell->get_mirrored_pair(green_goals[i%num_green_goals], agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
          managers.resource->spawn(RESOURCE_GREEN, pair.first);
          managers.resource->spawn(RESOURCE_RED, pair.second);
        }

        for (auto i = 0; i < num_fuel/2.0; i++){
          auto pair = managers.cell->get_mirrored_pair(green_goals[i%num_green_goals], agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
          managers.resource->spawn(FUEL, pair.first);
          managers.resource->spawn(FUEL, pair.second);
        }

        for (auto i = 0; i < num_obstacles/2.0; i++){
          auto pair = managers.cell->get_mirrored_pair(green_goals[i%num_green_goals], agent_pos, 2.0, 2.0, entities, float(managers.cell->get_world_dim())/2.0 -2.0 );
          managers.obstacle->spawn(pair.first);
          managers.obstacle->spawn(pair.second);
        }

        ship->set_pose(agent_pos, green_goals[0]);

      }

    };



  public:
    int ground_theme;
    std::unique_ptr<RoomGenerator> room_manager;

    std::shared_ptr<Ship> ship;

    int world_dim;
    int stat_dim;
    int bottom_dim;

    Managers managers;

    std::shared_ptr<InitLocator> init_locator;

    std::vector<float> state_ship;
    std::vector<float> state_goals;
    std::vector<float> state_resources;
    std::vector<float> state_obstacles;

    std::map<std::shared_ptr<Entity>,int> entity_to_state_idx;
    std::vector<int> hack_state_make_null_indices;


    Collector() : BasicAbstractGame() {

      register_obs_buffer("state_ship");
      register_obs_buffer("state_goals");
      register_obs_buffer("state_resources");
      register_obs_buffer("state_obstacles");

      options.register_option<int32_t>("world_dim",16);
      options.register_option<int32_t>("init_locator_type",2);
      options.register_option<int32_t>("num_goals_green",1);
      options.register_option<int32_t>("num_goals_red",1);
      options.register_option<int32_t>("num_resources_green",2);
      options.register_option<int32_t>("num_resources_red",2);
      options.register_option<int32_t>("num_fuel",2);
      options.register_option<int32_t>("num_obstacles",2);
      options.register_option<float>("goal_max",100.0);
      options.register_option<float>("goal_init",0.0);
      options.register_option<float>("agent_max_fuel",100.0);
      options.register_option<float>("agent_init_fuel",100.0);
      options.register_option<float>("agent_max_resources",100.0);
      options.register_option<float>("agent_init_resources_green",0.0);
      options.register_option<float>("agent_init_resources_red",0.0);


    }


    void load_background_images() override {
        main_bg_images_ptr = &space_backgrounds;
    }

    void asset_for_type(int type, std::vector<QString> &names) override {
        if (type == GOAL_RED) {
            names.push_back("misc_assets/ufoRed2.png");
        }else if (type == GOAL_GREEN) {
            names.push_back("misc_assets/ufoGreen2.png");
        }else if (type == RESOURCE_RED) {
            names.push_back("misc_assets/spaceEffect1_red.png");
        }else if (type == RESOURCE_GREEN) {
            names.push_back("misc_assets/spaceEffect1_green.png");
        }else if (type == FUEL) {
          names.push_back("misc_assets/spaceEffect1_blue.png");
        } else if (type == OBSTACLE) {
            names.push_back("misc_assets/meteorBrown_big1.png");
        } else if (type == PLAYER) {
            names.push_back("misc_assets/playerShip1_green.png");
        } else if (type == CAVEWALL) {
            names.push_back("misc_assets/groundA.png");
        } else if (type == EXHAUST) {
            names.push_back("misc_assets/towerDefense_tile295.png");
        }
    }


    void handle_agent_collision(const std::shared_ptr<Entity> &obj) override {
        BasicAbstractGame::handle_agent_collision(obj);

        if ((obj->type == RESOURCE_GREEN) || (obj->type == RESOURCE_RED)){
          auto resource = std::static_pointer_cast<Resource>(obj);

          float value = ship->resources->consume(resource);
          step_data.reward += value * ((obj->type == RESOURCE_GREEN) ? 1.0 : -1.0);
          if(resource->get_value() <= 0.0){
            resource->disappear();
            managers.resource->respawn(resource);
          }

        }else if(obj->type == FUEL){
          auto resource = std::static_pointer_cast<Resource>(obj);

          // ship->fuel->->consume(resource);
          ship->fuel->consume_greedy(resource);
          if(resource->get_value() <= 0.0){
            resource->disappear();
            managers.resource->respawn(resource);
          }
        }else if(obj->type == GOAL_GREEN){
          auto goal = std::static_pointer_cast<Goal>(obj);
          // goal->consume(ship->resources);
          float value = goal->consume(ship->resources);
          step_data.reward += value*100;
        }else if(obj->type == GOAL_RED){
          auto goal = std::static_pointer_cast<Goal>(obj);
          // goal->consume(ship->resources);
          float value = goal->consume(ship->resources);
          step_data.reward -= value*2*100;
        }
    }

    void update_agent_velocity() override {
        float v_scale = get_agent_acceleration_scale();

        float acc_x = maxspeed * action_vx * v_scale * .2;
        float acc_y = maxspeed * action_vy * v_scale * .2;

        float acc_mag = sqrt(pow(acc_x,2)+pow(acc_y,2));
        float vel_mag = sqrt(pow(agent->vx + mixrate * acc_x,2)+pow(agent->vy + mixrate * acc_y,2));

        if (ship->fuel->get_value() > 0.0){
          agent->vx += mixrate * acc_x;
          agent->vy += mixrate * acc_y;
        }

        if (acc_mag > 0.0){
          // step_data.reward -= vel_mag;
          ship->fuel->withdraw(vel_mag);
        }

        decay_agent_velocity();
    }

    bool use_block_asset(int type) override {
        return BasicAbstractGame::use_block_asset(type) || (type == CAVEWALL);
    }

    bool is_blocked(const std::shared_ptr<Entity> &src, int target, bool is_horizontal) override {
        if (BasicAbstractGame::is_blocked(src, target, is_horizontal))
            return true;
        if (src->type == PLAYER && target == CAVEWALL)
            return true;
        if (src->type == PLAYER && target == OBSTACLE)
            return true;

        return false;
    }


    void draw_gauge(QPainter &p, float x, float y, float h, float capacity, float val, QColor val_color){
      QPainterPath path;
      path.addRect(get_abs_rect(x-0.15/unit, y-0.15/unit, capacity+0.15/unit, h+0.15/unit));
      QPen pen(Qt::black, 0.15*unit);
      p.setPen(pen);
      p.fillPath(path, Qt::white);
      p.drawPath(path);
      p.fillRect(get_abs_rect(x, y, capacity*val, h), val_color);
    }

    void draw_entity(QPainter &p, const std::shared_ptr<Entity> &ent) override{
      BasicAbstractGame::draw_entity(p, ent);
      if (ent->type == GOAL_GREEN){
        auto goal = std::static_pointer_cast<Goal>(ent);
        draw_gauge(p, stat_dim/2.0+0.5, 1.0, .5, world_dim, goal->get_percentage(), QColor(  0, 200,   0));
      }else if (ent->type == GOAL_RED){
        auto goal = std::static_pointer_cast<Goal>(ent);
        draw_gauge(p, stat_dim/2.0+0.5, 2.0, .5, world_dim, goal->get_percentage(), QColor(200,   0,   0));
      }
    }


    void game_draw(QPainter &p, const QRect &rect) override {
        BasicAbstractGame::game_draw(p, rect);

        QColor red =   QColor(200,   0,   0);
        QColor green = QColor(  0, 200,   0);
        QColor blue =  QColor(  0,   0, 200);

        QPainterPath path;
        QPen pen;

        std::vector<std::pair<int,float>> res_percentages = ship->resources->get_percentages();

        float cell_offset = 0.0;
        float cell_size;
        QColor cell_color;
        for (auto cell : res_percentages){
          cell_size = cell.second*world_dim;
          cell_color = cell.first == RESOURCE_RED ? red : green;
          draw_gauge(p, stat_dim/2.0+0.5+cell_offset, 3.0, .5, cell_size, 1.0, cell_color);
          cell_offset += cell_size;
        }
        cell_color = Qt::white;
        cell_size = float(world_dim)-cell_offset;
        draw_gauge(p, stat_dim/2.0+0.5+cell_offset, 3.0, .5, cell_size, 0.0, cell_color);

        draw_gauge(p, stat_dim/2.0+0.5, 4.0, .5, world_dim, ship->fuel->get_percentage(), blue);
    }

    bool will_reflect(int src, int target) override {
        return BasicAbstractGame::will_reflect(src, target) || (src == ENEMY && (target == CAVEWALL || target == out_of_bounds_object));
    }

    void choose_world_dim() override {

        world_dim = options.get<int32_t>("world_dim");
        stat_dim = 5;
        bottom_dim = 1;

        main_width = world_dim+stat_dim+bottom_dim;
        main_height = world_dim+stat_dim+bottom_dim;
    }

    void game_init() override{
      BasicAbstractGame::game_init();

      mixrate = 0.9f;
      room_manager = std::make_unique<RoomGenerator>(this);
      managers.cell = std::make_shared<CellManager>(&rand_gen);
      managers.goal = std::make_shared<GoalManager>(&entities, options.get<float>("goal_init"), options.get<float>("goal_max")/1000.0, options.get<float>("goal_max"));
      managers.resource = std::make_shared<ResourceManager>(&entities);
      managers.obstacle = std::make_shared<ObstacleManager>(&entities);

      switch (options.get<int32_t>("init_locator_type")){
          case 1:
              init_locator = std::make_shared<InitLocatorRandom>(managers, &entities);
            break;
          case 2:
              init_locator = std::make_shared<InitLocatorSymmetric>(managers, &entities);
            break;
          case 3:
              init_locator = std::make_shared<InitLocatorInLine>(managers, &entities);
            break;

          default:
            std::cout << "Unknown locator type " << options.get<int32_t>("init_locator_type") << std::endl;
            fassert(false);
      }

      ship = std::make_shared<Ship>(options);
    }

    void game_reset() override {
        BasicAbstractGame::game_reset();

        options.center_agent = false;

        out_of_bounds_object = CAVEWALL;

        std::vector<int> space;
        float x_center = float(main_width)/2.0;
        float y_center = float(world_dim)/2.0 + bottom_dim;
        for (int i = 0; i < grid_size; i++) {
            float x = (i % main_width) + .5;
            float y = (i / main_height) + .5;
            if(sqrt(pow(x-x_center,2)+pow(y-y_center,2)) >= float(world_dim)/2.0){
              set_obj(i, CAVEWALL);
            }else{
              set_obj(i, SPACE);
              space.push_back(i);
            }
        }

        managers.cell->reset(main_width,main_height);
        managers.cell->add_cells(space.begin(),space.end());
        managers.cell->randomize();

        ship->reset(agent);
        managers.goal->reset();
        managers.resource->reset();
        managers.obstacle->reset();

        init_locator->init(ship, options);



        // int num_goals_green = options.get<int32_t>("num_goals_green");
        // int num_goals_red = options.get<int32_t>("num_goals_red");
        // int num_resources_green = options.get<int32_t>("num_resources_green");
        // int num_resources_red = options.get<int32_t>("num_resources_red");
        // int num_fuel = options.get<int32_t>("num_fuel");
        // int num_obstacles = options.get<int32_t>("num_obstacles");


        // int goals_green_offset = 9;
        // int goals_red_offset = goals_green_offset + num_goals_green*3;
        // int resources_green_offset = goals_red_offset + num_goals_red*3;
        // int resources_red_offset = resources_green_offset + num_resources_green*3;
        // int fuel_offset = resources_red_offset + num_resources_red*3;
        // int obstacles_offset = fuel_offset + num_fuel*3;


        // entity_to_state_idx.clear();
        // hack_state_make_null_indices.clear();
        // state.resize(obstacles_offset+num_obstacles*3,0.0);

        state_ship.clear();
        state_goals.clear();
        state_resources.clear();
        state_obstacles.clear();
        ship->push_state_to_vector(state_ship);
        managers.goal->push_state_to_vector(state_goals);
        managers.resource->push_state_to_vector(state_resources);
        managers.obstacle->push_state_to_vector(state_obstacles);

        auto ptr_state_ship_obs = point_to_obs<float>("state_ship");
        if (ptr_state_ship_obs != 0){
          for (uint32_t i =0; i < state_ship.size(); i++){
            ptr_state_ship_obs[i] = state_ship[i];
          }
        }

        auto ptr_state_goals_obs = point_to_obs<float>("state_goals");
        if (ptr_state_goals_obs != 0){
          for (uint32_t i =0; i < state_goals.size(); i++){
            ptr_state_goals_obs[i] = state_goals[i];
          }
        }

        auto ptr_state_resources_obs = point_to_obs<float>("state_resources");
        if (ptr_state_resources_obs != 0){
          for (uint32_t i =0; i < state_resources.size(); i++){
            ptr_state_resources_obs[i] = state_resources[i];
          }
        }

        auto ptr_state_obstacles_obs = point_to_obs<float>("state_obstacles");
        if (ptr_state_obstacles_obs != 0){
          for (uint32_t i =0; i < state_obstacles.size(); i++){
            ptr_state_obstacles_obs[i] = state_obstacles[i];
          }
        }



        //
        // state[0] = agent->x;
        // state[1] = agent->y;
        // state[2] = agent->rotation;
        // state[3] = agent->vx;
        // state[4] = agent->vy;
        // state[5] = agent->vrot;
        // state[6] = ship->fuel->get_value();
        // state[7] = ship->resources->get_value(RESOURCE_GREEN);
        // state[8] = ship->resources->get_value(RESOURCE_RED);
        //
        //
        // // std::cout << "AAA" << std::endl;
        // // for (auto xxx : entity_to_state_idx){
        // //   std::cout << "\t " << xxx.first << "[ " << xxx.first->type << "] : " << xxx.second << std::endl;
        // // }
        // //
        // // std::cout << "BBB" << std::endl;
        // for (auto ent : entities){
        //   if (ent->type == GOAL_GREEN){
        //     entity_to_state_idx[ent] = goals_green_offset;
        //     state[entity_to_state_idx[ent]] = ent->x;
        //     state[entity_to_state_idx[ent]+1] = ent->y;
        //     auto goal = std::static_pointer_cast<Goal>(ent);
        //     state[entity_to_state_idx[ent]+2] = goal->get_value();
        //     goals_green_offset += 3;
        //   }else if (ent->type == GOAL_RED){
        //     entity_to_state_idx[ent] = goals_red_offset;
        //     state[entity_to_state_idx[ent]] = ent->x;
        //     state[entity_to_state_idx[ent]+1] = ent->y;
        //     auto goal = std::static_pointer_cast<Goal>(ent);
        //     state[entity_to_state_idx[ent]+2] = goal->get_value();
        //     goals_red_offset += 3;
        //   }else if (ent->type == RESOURCE_GREEN){
        //     entity_to_state_idx[ent] = resources_green_offset;
        //     state[entity_to_state_idx[ent]] = ent->x;
        //     state[entity_to_state_idx[ent]+1] = ent->y;
        //     state[entity_to_state_idx[ent]+2] = 1.0;
        //     hack_state_make_null_indices.push_back(entity_to_state_idx[ent]+2);
        //     resources_green_offset += 3;
        //   }else if (ent->type == RESOURCE_RED){
        //     entity_to_state_idx[ent] = resources_red_offset;
        //     state[entity_to_state_idx[ent]] = ent->x;
        //     state[entity_to_state_idx[ent]+1] = ent->y;
        //     state[entity_to_state_idx[ent]+2] = 1.0;
        //     hack_state_make_null_indices.push_back(entity_to_state_idx[ent]+2);
        //     resources_red_offset += 3;
        //   }else if (ent->type == FUEL){
        //     entity_to_state_idx[ent] = fuel_offset;
        //     state[entity_to_state_idx[ent]] = ent->x;
        //     state[entity_to_state_idx[ent]+1] = ent->y;
        //     state[entity_to_state_idx[ent]+2] = 1.0;
        //     hack_state_make_null_indices.push_back(entity_to_state_idx[ent]+2);
        //     fuel_offset += 3;
        //   }else if (ent->type == OBSTACLE){
        //     entity_to_state_idx[ent] = obstacles_offset;
        //     state[entity_to_state_idx[ent]] = ent->x;
        //     state[entity_to_state_idx[ent]+1] = ent->y;
        //     state[entity_to_state_idx[ent]+2] = 1.0;
        //     hack_state_make_null_indices.push_back(entity_to_state_idx[ent]+2);
        //     obstacles_offset += 3;
        //   }
        // }


        // std::cout << "CCC" << std::endl;
        // for (auto xxx : entity_to_state_idx){
        //   std::cout << "\t " << xxx.first << "[ " << xxx.first->type << "] : " << xxx.second << std::endl;
        // }
        // fassert(false);
    }

    void set_action_xy(int move_act) override {
        float acceleration = move_act % 3 - 1;
        if (acceleration < 0){
            acceleration *= 0.33f;
        }

        float theta = -1 * agent->rotation + PI / 2;

        if (acceleration > 0) {
            auto exhaust = add_entity(agent->x - agent->rx * cos(theta), agent->y - agent->ry * sin(theta), 0, 0, .5 * agent->rx, EXHAUST);
            exhaust->expire_time = 4;
            exhaust->rotation = -1 * theta - PI / 2;
            exhaust->grow_rate = 1.25;
            exhaust->alpha_decay = 0.8f;
        }

        action_vy = acceleration * sin(theta);
        action_vx = acceleration * cos(theta);
        action_vrot = move_act / 3 - 1;
    }

    void game_step() override {
        BasicAbstractGame::game_step();

        // for (auto goal : managers.goal->get_goals(GOAL_GREEN)){
        //   step_data.reward -= sqrt(pow(agent->x-goal->x,2)+pow(agent->y-goal->y,2));
        // }
        //
        // for (auto goal : managers.goal->get_goals(GOAL_RED)){
        //   step_data.reward -= (float(world_dim) - sqrt(pow(agent->x-goal->x,2)+pow(agent->y-goal->y,2)))/float(world_dim);
        //   step_data.reward -= goal->get_percentage();
        // }

        // step_data.reward -= (1.0-(ship->fuel->get_percentage()));
        // step_data.reward -= 0.1;


        if (ship->fuel->get_value() < 0.0+1e-10){
          step_data.done = true;
          step_data.level_complete = false;
        }

        // std::cout << "pre hack" << std::endl;
        // for (auto xxx : entity_to_state_idx){
        //   std::cout << "\t " << xxx.first << "[ " << xxx.first->type << "] : " << xxx.second << std::endl;
        // }
        // std::cout << "agent " << state[0] << " " << state[1] << " " << state[2] << " " << state[3] << " " << state[4] << " " << state[5] << " " << state[6] << " " << state[7] << " " << state[8] << std::endl;
        // for (uint32_t i = 9; i < state.size(); i += 3){
        //   std::cout << "ent [?|"<<i<<"] " << state[i] << " " << state[i+1] << ": " << state[i+2] << std::endl;
        // }
        //
        // std::cout << "hack" << std::endl;
        // for (auto i : hack_state_make_null_indices){
        //   state[i] = 0.0;
        // }

        // std::cout << "post hack" << std::endl;
        // for (auto xxx : entity_to_state_idx){
        //   std::cout << "\t " << xxx.first << "[ " << xxx.first->type << "] : " << xxx.second << std::endl;
        // }
        // std::cout << "agent " << state[0] << " " << state[1] << " " << state[2] << " " << state[3] << " " << state[4] << " " << state[5] << " " << state[6] << " " << state[7] << " " << state[8] << std::endl;
        // for (uint32_t i = 9; i < state.size(); i += 3){
        //   std::cout << "ent [?|"<<i<<"] " << state[i] << " " << state[i+1] << ": " << state[i+2] << std::endl;
        // }


        for (auto ent : entities){
          if ((ent->type == GOAL_RED) || (ent->type == GOAL_GREEN)) {
            auto goal = std::static_pointer_cast<Goal>(ent);
            if (goal->get_percentage() > 1.0-1e-10){
              step_data.done = true;
              step_data.level_complete = true;
            }
          }
          //
          // auto it = entity_to_state_idx.find(ent);
          // if (it != entity_to_state_idx.end()){
          //   int sdx = entity_to_state_idx[ent];
          //   if((state[sdx] != ent->x) ||(state[sdx+1] != ent->y)){
          //     std::cout << "sw ent[" << ent << "|" << sdx << "] " << state[sdx] << "<>" << ent->x << " " << state[sdx+1] << "<>" << ent->y << std::endl;
          //     fassert(false);
          //   }
          //   if ((ent->type == GOAL_RED) || (ent->type == GOAL_GREEN)) {
          //     auto goal = std::static_pointer_cast<Goal>(ent);
          //     state[sdx+2] = goal->get_value();
          //   }else{
          //     state[sdx+2] = 1.0;
          //   }
          // }
        }

        state_ship.clear();
        state_goals.clear();
        state_resources.clear();
        state_obstacles.clear();

        ship->push_state_to_vector(state_ship);
        managers.goal->push_state_to_vector(state_goals);
        managers.resource->push_state_to_vector(state_resources);
        managers.obstacle->push_state_to_vector(state_obstacles);


        // std::cout << "post loop" << std::endl;
        // for (auto xxx : entity_to_state_idx){
        //   std::cout << "\t " << xxx.first << "[ " << xxx.first->type << "] : " << xxx.second << std::endl;
        // }
        //
        // std::cout << "agent " << state[0] << " " << state[1] << " " << state[2] << " " << state[3] << " " << state[4] << " " << state[5] << " " << state[6] << " " << state[7] << " " << state[8] << std::endl;
        // for (uint32_t i = 9; i < state.size(); i += 3){
        //   std::cout << "ent [?|"<<i<<"] " << state[i] << " " << state[i+1] << ": " << state[i+2] << std::endl;
        // }

        auto ptr_state_ship_obs = point_to_obs<float>("state_ship");
        if (ptr_state_ship_obs != 0){
          for (uint32_t i =0; i < state_ship.size(); i++){
            ptr_state_ship_obs[i] = state_ship[i];
          }
        }

        auto ptr_state_goals_obs = point_to_obs<float>("state_goals");
        if (ptr_state_goals_obs != 0){
          for (uint32_t i =0; i < state_goals.size(); i++){
            ptr_state_goals_obs[i] = state_goals[i];
          }
        }

        auto ptr_state_resources_obs = point_to_obs<float>("state_resources");
        if (ptr_state_resources_obs != 0){
          for (uint32_t i =0; i < state_resources.size(); i++){
            ptr_state_resources_obs[i] = state_resources[i];
          }
        }

        auto ptr_state_obstacles_obs = point_to_obs<float>("state_obstacles");
        if (ptr_state_obstacles_obs != 0){
          for (uint32_t i =0; i < state_obstacles.size(); i++){
            ptr_state_obstacles_obs[i] = state_obstacles[i];
          }
        }


        step_data.reward -= 10.0;

        if (step_data.done){
          step_data.reward -= (1.0-(ship->fuel->get_percentage()))*100;
        }
        // if (step_data.done){
        //   // step_data.reward -= float(cur_time)/float(timeout)*10;
        //   step_data.reward -= (1.0-(ship->fuel->get_percentage()));
        //
        //   for (auto goal : managers.goal->get_goals(GOAL_GREEN)){
        //     step_data.reward -= (1.0-goal->get_percentage());
        //   }
        //
        //   for (auto goal : managers.goal->get_goals(GOAL_RED)){
        //     step_data.reward -= goal->get_percentage()*2.0;
        //   }
        //
        //   if (step_data.done){
        //     if (ship->fuel->get_percentage() < 0.05){
        //       step_data.reward *= float(timeout-cur_time);
        //     }else{
        //       for (auto goal : managers.goal->get_goals(GOAL_RED)){
        //         if (goal->get_percentage() >= 0.5){
        //           step_data.reward *= float(timeout-cur_time);
        //           break;
        //         }
        //       }
        //     }
        //   }
        //
        //   step_data.reward *= 10;
        // // }

        erase_if_needed();
    }
};

REGISTER_GAME("collector", Collector);
