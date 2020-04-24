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

    std::pair<float,float> get_mirrored_point(const std::pair<float,float> &src, const std::pair<float,float> &mirror_point){
      std::pair<float,float> target(2.0*mirror_point.first-src.first,2.0*mirror_point.second-src.second);
      pop_cell(pos_to_cell(target.first, target.second));
      return target;
    }

    std::pair<float,float> get_mirrored_point(const std::pair<float,float> &src){
      std::pair<float,float> mirror_point((main_width-5.0-1.0)/2.0 + (5.0+1.0)/2.0, (main_height-5.0-1.0)/2.0 + 1.0);
      return get_mirrored_point(src, mirror_point);
    }

    std::pair<float,float> get_projection_on_line(const std::pair<float,float> &src, const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b){
      float a = mirror_line_b.second-mirror_line_a.second;
      float b = mirror_line_a.first-mirror_line_b.first;
      float c = mirror_line_b.first*mirror_line_a.second-mirror_line_a.first*mirror_line_b.second;
      float d = pow(a,2) + pow(b,2);
      return std::pair<float,float> ((b*(b*src.first-a*src.second)-a*c)/d,(a*(a*src.second-b*src.first)-b*c)/d);
    }

    std::pair<float,float> get_mirrored_point(const std::pair<float,float> &src, const std::pair<float,float> &mirror_line_a, const std::pair<float,float> &mirror_line_b){
      return get_mirrored_point(src, get_projection_on_line(src, mirror_line_a, mirror_line_b));
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
      std::vector<int> resource_types;

    public:
      // std::make_shared<Entity>(src->x, src->y, vx, vy, obj_r, type);

      Goal(float _init_value, float _step_value, float _max_value, float _x, float _y, float _vx, float _vy, float _r, int _type): Entity(_x, _y, _vx, _vy, _r, _type), value(_init_value), init_value(_init_value), step_value(_step_value), max_value(_max_value){
      }

      void add_resource_type(int resource_type){
        resource_types.push_back(resource_type);
      }

      bool accepts_resource_type(int resource_type){
        std::vector<int>::iterator it = std::find(resource_types.begin(), resource_types.end(), resource_type);
        return (it != resource_types.end());
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
        if (accepts_resource_type(resource_type)){
          deposit(net_value);
        }
        return net_value;
      }

      float consume(const std::shared_ptr<ResourceContainerSlotted> &container){
        float net_value = container->get_value();
        while (container->get_value() > 0.0){
          std::pair<int,float> slot = container->withdraw_last_slot();
          if (accepts_resource_type(slot.first)){
            deposit(slot.second);
          }
        }
        return net_value;
      }



      void reset(){
        value = 0.0;
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
      std::shared_ptr<CellManager> free_cells;
      std::vector<std::shared_ptr<Entity> > *entities;

    public:
    ResourceManager(std::shared_ptr<CellManager> &_free_cells, std::vector<std::shared_ptr<Entity> > *_entities): free_cells(_free_cells), entities(_entities){
    }

    void spawn(int resource_type, int cell){
      std::pair<float,float> pos = free_cells->cell_to_pos(cell);
      auto resource = std::make_shared<Resource>(0.0, 1.0, 10.0, pos.first, pos.second, 0.0, 0.0, 0.5, resource_type);
      resource->collides_with_entities = true;
      entities->push_back(resource);
    }

    void respawn(const std::shared_ptr<Resource> &resource){
      std::cerr << "RESPAWN ME" << std::endl;
    }

    void respawn(const std::shared_ptr<Entity> &entity){
      if ((entity->type == RESOURCE_GREEN) || (entity->type == RESOURCE_RED) || (entity->type == FUEL)){
        respawn(std::static_pointer_cast<Resource>(entity));
      }
    }

  };

  class InitLocator {

  protected:
    std::shared_ptr<CellManager> free_cells;
    std::vector<std::shared_ptr<Entity> > *entities;
    std::shared_ptr<ResourceManager> resources;
public:
    InitLocator(std::shared_ptr<CellManager> &_free_cells, std::vector<std::shared_ptr<Entity> > *_entities, std::shared_ptr<ResourceManager> &_resources): free_cells(_free_cells), entities(_entities), resources(_resources){
    }

    virtual void init(const std::shared_ptr<Entity> &agent, const std::vector<std::pair<int,int> > &spawn_list) = 0;

    virtual ~InitLocator(){

    }

  };

  class InitLocatorRandom : public InitLocator {

  public:
    InitLocatorRandom(std::shared_ptr<CellManager> &_free_cells, std::vector<std::shared_ptr<Entity> > *_entities, std::shared_ptr<ResourceManager> &_resources) : InitLocator(_free_cells, _entities, _resources){
    }

    virtual void init(const std::shared_ptr<Entity> &agent, const std::vector<std::pair<int,int> > &spawn_list) override{

      for (auto type_num : spawn_list ){

        switch (type_num.first){
          case RESOURCE_GREEN:
          case RESOURCE_RED:
          case FUEL:
              for (int i=0; i<type_num.second; i++){
                resources->spawn(type_num.first, free_cells->pop_random());
              }
            break;
          case GOAL_RED:
          case GOAL_GREEN:
            for (int i=0; i<type_num.second; i++){
              std::pair<float,float> pos = free_cells->cell_to_pos(free_cells->pop_random());
              auto goal = std::make_shared<Goal>(0.0, 1.0, 1000.0, pos.first, pos.second, 0.0, 0.0, 0.5, type_num.first);
              goal->collides_with_entities = true;
              goal->add_resource_type( type_num.first == GOAL_RED ? RESOURCE_RED : RESOURCE_GREEN );
              entities->push_back(goal);
            }
            break;
          case OBSTACLE:
            for (int i=0; i<type_num.second; i++){
              std::pair<float,float> pos = free_cells->cell_to_pos(free_cells->pop_random());
              auto obstacle = std::make_shared<Entity>(pos.first, pos.second, 0.0, 0.0, 0.5, type_num.first);
              obstacle->collides_with_entities = true;
              entities->push_back(obstacle);
            }
            break;
          default:
            std::cerr << "Unknown entity type: " << type_num.first << std::endl;
            break;
        }
      }

      std::pair<float,float> pos = free_cells->cell_to_pos(free_cells->pop_random());
      agent->x = pos.first;
      agent->y = pos.second;

    }

  };


  class InitLocatorSymmetric : public InitLocator {

  public:
    InitLocatorSymmetric(std::shared_ptr<CellManager> &_free_cells, std::vector<std::shared_ptr<Entity> > *_entities, std::shared_ptr<ResourceManager> &_resources) : InitLocator(_free_cells, _entities, _resources){
    }

    virtual void init(const std::shared_ptr<Entity> &agent, const std::vector<std::pair<int,int> > &spawn_list) override{


      std::pair<float,float> goal_green_pos = free_cells->cell_to_pos(free_cells->pop_random());
      std::pair<float,float> goal_red_pos = free_cells->get_mirrored_point(goal_green_pos);


      for (auto type_num : spawn_list ){

        switch (type_num.first){
          case RESOURCE_GREEN:
          case RESOURCE_RED:
          case FUEL:
              for (int i=0; i<type_num.second; i++){
                // resources->spawn(type_num.first, free_cells->pop_random());
              }
            break;
          case GOAL_RED:
          case GOAL_GREEN:
            for (int i=0; i<type_num.second; i++){
              std::pair<float,float> pos = type_num.first == GOAL_GREEN ? goal_green_pos : goal_red_pos;
              // std::pair<float,float> pos = free_cells->cell_to_pos(free_cells->pop_random());
              auto goal = std::make_shared<Goal>(0.0, 1.0, 1000.0, pos.first, pos.second, 0.0, 0.0, 0.5, type_num.first);
              goal->collides_with_entities = true;
              goal->add_resource_type( type_num.first == GOAL_RED ? RESOURCE_RED : RESOURCE_GREEN );
              entities->push_back(goal);
            }
            break;
          case OBSTACLE:
            for (int i=0; i<type_num.second; i++){
              // std::pair<float,float> pos = free_cells->cell_to_pos(free_cells->pop_random());
              // auto obstacle = std::make_shared<Entity>(pos.first, pos.second, 0.0, 0.0, 0.5, type_num.first);
              // obstacle->collides_with_entities = true;
              // entities->push_back(obstacle);
            }
            break;
          default:
            std::cerr << "Unknown entity type: " << type_num.first << std::endl;
            break;
        }
      }

      std::pair<float,float> pos = free_cells->cell_to_pos(free_cells->pop_random());
      agent->x = pos.first;
      agent->y = pos.second;

    }

  };



  public:
    int ground_theme;
    std::unique_ptr<RoomGenerator> room_manager;

    std::shared_ptr<Goal> goal_green;
    std::shared_ptr<Goal> goal_red;

    std::shared_ptr<ResourceContainer> fuel_container;
    std::shared_ptr<ResourceContainerSlotted> resource_container;



    int world_dim;
    int stat_dim;
    int bottom_dim;

    int counter;

    std::shared_ptr<CellManager> free_cell_manager;
    std::shared_ptr<ResourceManager> resource_manager;
    std::shared_ptr<InitLocator> init_locator;


    Collector() : BasicAbstractGame() {
        mixrate = 0.9f;
        room_manager = std::make_unique<RoomGenerator>(this);
        free_cell_manager = std::make_shared<CellManager>(&rand_gen);
        resource_manager = std::make_shared<ResourceManager>(free_cell_manager, &entities);
        // init_locator = std::make_shared<InitLocatorRandom>(free_cell_manager, &entities, resource_manager);
        init_locator = std::make_shared<InitLocatorSymmetric>(free_cell_manager, &entities, resource_manager);

        fuel_container = std::make_shared<ResourceContainer>(100.0, 100.0);
        resource_container = std::make_shared<ResourceContainerSlotted>(100.0, 0.0);

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

    // void respawn_resource(const std::shared_ptr<Entity> &obj){
    //   float x;
    //   float y;
    //   int idx = 0;
    //
    //   std::vector<int> selected_idxs = rand_gen.simple_choose((int)(free_cells.size()), (int)(free_cells.size()));
    //
    //   for (int i : selected_idxs){
    //     idx = i;
    //     x = (free_cells[idx] % main_width) + .5;
    //     y = (free_cells[idx] / main_width) + .5;
    //
    //     if (sqrt(pow(x-agent->x,2)+pow(y-agent->y,2)) > 3.0){
    //       break;
    //     }
    //   }
    //
    //   int old_idx = int(obj->y) * main_width + int(obj->x);
    //   free_cells.erase(free_cells.begin()+idx);
    //   free_cells.push_back(old_idx);
    //
    //   obj->x = x;
    //   obj->y = y;
    // }

    void handle_agent_collision(const std::shared_ptr<Entity> &obj) override {
        BasicAbstractGame::handle_agent_collision(obj);

        if ((obj->type == RESOURCE_GREEN) || (obj->type == RESOURCE_RED)){
          auto resource = std::static_pointer_cast<Resource>(obj);

          resource_container->consume(resource);
          if(resource->get_value() <= 0.0){
            resource->disappear();
            resource_manager->respawn(resource);
          }
        }else if(obj->type == FUEL){
          auto resource = std::static_pointer_cast<Resource>(obj);

          fuel_container->consume(resource);
          if(resource->get_value() <= 0.0){
            resource->disappear();
            resource_manager->respawn(resource);
          }
        }else if(obj->type == GOAL_GREEN){

          auto goal = std::static_pointer_cast<Goal>(obj);
          goal->consume(resource_container);
        }else if(obj->type == GOAL_RED){
          auto goal = std::static_pointer_cast<Goal>(obj);
          goal->consume(resource_container);
        }



        // if (obj->type == RESOURCE_GREEN){
        //   if ( agent_resources_green + agent_resources_red < MAX_RESOURCES ){
        //     agent_resources_green += 1;
        //     // set_obj(int(obj->x),int(obj->y),SPACE);
        //     //create a new resource
        //   }
        // }else if (obj->type == RESOURCE_RED){
        //   if ( agent_resources_green + agent_resources_red < MAX_RESOURCES ){
        //     agent_resources_red += 1;
        //     // set_obj(int(obj->x),int(obj->y),SPACE);
        //     //create a new resource
        //   }
        // } else if (obj->type == GOAL_GREEN) {
        //     goal_resources_green += agent_resources_green;
        //     agent_resources_green = 0;
        //     agent_resources_red = 0;
        // } else if (obj->type == GOAL_RED) {
        //     goal_resources_red += agent_resources_red;
        //     agent_resources_green = 0;
        //     agent_resources_red = 0;
        // }

        //
        // if (obj->type == GOAL) {
        //     step_data.reward += GOAL_REWARD;
        //     step_data.level_complete = true;
        //     step_data.done = true;
        // } else if (obj->type == ENEMY) {
        //     step_data.done = true;
        // } else if (obj->type == TARGET) {
        //     step_data.done = true;
        // }
    }

    void update_agent_velocity() override {
        float v_scale = get_agent_acceleration_scale();

        float acc_x = maxspeed * action_vx * v_scale * .2;
        float acc_y = maxspeed * action_vy * v_scale * .2;

        float acc_mag = sqrt(pow(acc_x,2)+pow(acc_y,2));
        float vel_mag = sqrt(pow(agent->vx + mixrate * acc_x,2)+pow(agent->vy + mixrate * acc_y,2));

        if (acc_mag > 0.0){
          fuel_container->withdraw(vel_mag);
        }

        if (fuel_container->get_value() > 0.0){
          agent->vx += mixrate * acc_x;
          agent->vy += mixrate * acc_y;
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


    void draw_gauge(QPainter &p, float x, float y, float h, float capacity, float state, QColor state_color){
      QPainterPath path;
      path.addRect(get_abs_rect(x-0.15/unit, y-0.15/unit, capacity+0.15/unit, h+0.15/unit));
      QPen pen(Qt::black, 0.15*unit);
      p.setPen(pen);
      p.fillPath(path, Qt::white);
      p.drawPath(path);
      p.fillRect(get_abs_rect(x, y, capacity*state, h), state_color);
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

        std::vector<std::pair<int,float>> res_percentages = resource_container->get_percentages();

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

        draw_gauge(p, stat_dim/2.0+0.5, 4.0, .5, world_dim, fuel_container->get_percentage(), blue);
    }

    void handle_collision(const std::shared_ptr<Entity> &src, const std::shared_ptr<Entity> &target) override {
      //
      // if ((src->type == RESOURCE_GREEN)&&(target->type == PLAYER)){
      //   std::cout << "src: " << src->type << "target: " << target->type << std::endl;
      //
      //   std::vector<int> free_cells;
      //
      //   float x;
      //   float y;
      //
      //   for (int i = 0; i < grid_size; i++) {
      //       if (get_obj(i) == SPACE) {
      //
      //           x = (i % main_width) + .5;
      //           y = (i / main_width) + .5;
      //
      //           if (sqrt(pow(agent->x-x,2) + pow(agent->y-y,2) > 3.0)){
      //             free_cells.push_back(i);
      //           }
      //
      //       }
      //   }
      //
      //   std::vector<int> selected_idxs = rand_gen.simple_choose((int)(free_cells.size()), 1);
      //
      //   src->x = (selected_idxs[0] % main_width) + .5;
      //   src->y = (selected_idxs[0] / main_width) + .5;
      //
      //
      // }else if ((target->type == RESOURCE_GREEN)&&(src->type == PLAYER)){
      //
      //   std::cout << "src: " << src->type << "target: " << target->type << std::endl;
      //
      //   std::vector<int> free_cells;
      //
      //   float x;
      //   float y;
      //
      //   for (int i = 0; i < grid_size; i++) {
      //       if (get_obj(i) == SPACE) {
      //
      //           x = (i % main_width) + .5;
      //           y = (i / main_width) + .5;
      //
      //           if (sqrt(pow(agent->x-x,2) + pow(agent->y-y,2) > 3.0)){
      //             free_cells.push_back(i);
      //           }
      //
      //       }
      //   }
      //
      //   std::vector<int> selected_idxs = rand_gen.simple_choose((int)(free_cells.size()), 1);
      //
      //   target->x = (selected_idxs[0] % main_width) + .5;
      //   target->y = (selected_idxs[0] / main_width) + .5;
      //
      // }

        // if (target->type == PLAYER_BULLET) {
        //     bool erase_bullet = false;
        //
        //     if (src->type == TARGET) {
        //         src->health -= 1;
        //         erase_bullet = true;
        //
        //         if (src->health <= 0 && !src->will_erase) {
        //             spawn_child(src, EXPLOSION, .5 * src->rx);
        //             src->will_erase = true;
        //             step_data.reward += TARGET_REWARD;
        //         }
        //     } else if (src->type == OBSTACLE) {
        //         erase_bullet = true;
        //     } else if (src->type == ENEMY) {
        //         erase_bullet = true;
        //     } else if (src->type == GOAL) {
        //         erase_bullet = true;
        //     }
        //
        //     if (erase_bullet && !target->will_erase) {
        //         target->will_erase = true;
        //         auto explosion = spawn_child(target, EXPLOSION, .5 * target->rx);
        //         explosion->vx = src->vx;
        //         explosion->vy = src->vy;
        //     }
        // }
    }

    bool will_reflect(int src, int target) override {
        return BasicAbstractGame::will_reflect(src, target) || (src == ENEMY && (target == CAVEWALL || target == out_of_bounds_object));
    }

    void choose_world_dim() override {
        DistributionMode dist_diff = options.distribution_mode;

        world_dim = 20;
        stat_dim = 5;
        bottom_dim = 1;

        if (dist_diff == EasyMode) {
            world_dim = 30;
        } else if (dist_diff == HardMode) {
            world_dim = 40;
        } else if (dist_diff == MemoryMode) {
            world_dim = 60;
        }

        main_width = world_dim+stat_dim+bottom_dim;
        main_height = world_dim+stat_dim+bottom_dim;
    }


    // std::vector<int> get_free_cells(int num, bool in_order, ){
    //   fassert(num <= free_cells.size());
    //   std::vector<int> res(free_cells.begin(),free_cells.begin()+num)
    //   return res;
    // }


    void game_reset() override {
        BasicAbstractGame::game_reset();



        options.center_agent = false;
        resource_container->reset();

        fuel_container->reset(fuel_container->get_max());

        counter = 0;

        // goal_green->reset();
        // goal_red->reset();

        out_of_bounds_object = CAVEWALL;

        std::vector<int> space;
        float x_center = main_width/2.0;
        float y_center = world_dim/2.0 + bottom_dim;
        for (int i = 0; i < grid_size; i++) {
            float x = (i % main_width) + .5;
            float y = (i / main_height) + .5;
            // std::cout << " iii " <<  x << "," << y << " | " << main_width <<std::endl;
            if(sqrt(pow(x-x_center,2)+pow(y-y_center,2)) >= world_dim/2.0){
              set_obj(i, CAVEWALL);
              // std::cout << "WALL" << std::endl;
            }else{
              set_obj(i, SPACE);
              // std::cout << "SPACE" << std::endl;
              space.push_back(i);
            }
        }

        free_cell_manager->reset(main_width,main_height);
        free_cell_manager->add_cells(space.begin(),space.end());
        free_cell_manager->randomize();

        std::cout << " DONE building " << free_cell_manager->size() << std::endl;

        // for (int iteration = 0; iteration < 4; iteration++) {
        //     room_manager->update();
        // }
        //
        // std::set<int> best_room;
        // room_manager->find_best_room(best_room);
        // fassert(best_room.size() > 0);
        //
        // for (int i = 0; i < grid_size; i++) {
        //     set_obj(i, WALL_OBJ);
        // }



        // for (int i : best_room) {
        //     set_obj(i, SPACE);
        //     free_cells.push_back(i);
        // }

        // std::vector<int> selected_idxs = rand_gen.simple_choose((int)(free_cells.size()), 3);


        // int agent_cell = free_cells.front();
        // free_cells.erase(free_cells.begin());
        // int goal_cell_green = free_cells.front();
        // free_cells.erase(free_cells.begin());
        // int goal_cell_red = free_cells.front();
        // free_cells.erase(free_cells.begin());
        //
        // agent->x = (agent_cell % main_width) + .5;
        // agent->y = (agent_cell / main_height) + .5;
        //
        // auto ent_green = spawn_entity_at_idx(goal_cell_green, .5, GOAL_GREEN);
        // ent_green->collides_with_entities = true;
        //
        // auto ent_red = spawn_entity_at_idx(goal_cell_red, .5, GOAL_RED);
        // ent_red->collides_with_entities = true;




        // std::vector<int> goal_path_green;
        // room_manager->find_path(agent_cell, goal_cell_green, goal_path_green);

        // std::vector<int> goal_path_red;
        // room_manager->find_path(agent_cell, goal_cell_red, goal_path_red);

        // bool should_prune = options.distribution_mode != MemoryMode;

        // if (should_prune) {
        //     std::set<int> wide_path_green;
        //     wide_path_green.insert(goal_path_green.begin(), goal_path_green.end());
        //     room_manager->expand_room(wide_path_green, 4);
        //
        //     std::set<int> wide_path_red;
        //     wide_path_red.insert(goal_path_red.begin(), goal_path_red.end());
        //     room_manager->expand_room(wide_path_red, 4);
        //
        //
        //     for (int i = 0; i < grid_size; i++) {
        //         set_obj(i, WALL_OBJ);
        //     }
        //
        //     for (int i : wide_path_green) {
        //         set_obj(i, SPACE);
        //     }
        //
        //     for (int i : wide_path_red) {
        //         set_obj(i, SPACE);
        //     }
        // }

        // for (int iteration = 0; iteration < 4; iteration++) {
        //     room_manager->update();
        //
        //     for (int i : goal_path_green) {
        //         set_obj(i, SPACE);
        //     }
        //
        //     for (int i : goal_path_red) {
        //         set_obj(i, SPACE);
        //     }
        // }
        //
        // for (int i : goal_path_green) {
        //     set_obj(i, MARKER);
        // }
        //
        // for (int i : goal_path_red) {
        //     set_obj(i, MARKER);
        // }

        // free_cells.clear();

        // for (int i = 0; i < grid_size; i++) {
            // if (get_obj(i) == SPACE) {
            //     free_cells.push_back(i);
            // } else
            // if (get_obj(i) == WALL_OBJ) {
                // set_obj(i, CAVEWALL);
            // }
        // }

        int chunk_size = free_cell_manager->size() / 80;

        std::vector<std::pair<int,int>> objs;
        objs.push_back(std::pair<int,int>(GOAL_GREEN,1));
        objs.push_back(std::pair<int,int>(GOAL_RED,1));
        objs.push_back(std::pair<int,int>(OBSTACLE,chunk_size));
        objs.push_back(std::pair<int,int>(FUEL,chunk_size));
        objs.push_back(std::pair<int,int>(RESOURCE_RED,chunk_size));
        objs.push_back(std::pair<int,int>(RESOURCE_GREEN,chunk_size));

        init_locator->init(agent, objs);

        // int num_objs = 4 * chunk_size;
        //
        // // std::vector<int> obj_idxs = rand_gen.simple_choose((int)(free_cells.size()), num_objs);
        //
        // for (int i = 0; i < num_objs; i++) {
        //     // int val = free_cells[obj_idxs[i]];
        //     int val = free_cells.front();
        //     free_cells.erase(free_cells.begin());
        //
        //     if (i < chunk_size) {
        //       auto e = spawn_entity_at_idx(val, .5, OBSTACLE);
        //       e->collides_with_entities = true;
        //       // set_obj(val, OBSTACLE);
        //     } else if (i < 2 * chunk_size) {
        //       auto e = spawn_entity_at_idx(val, .5, FUEL);
        //       e->collides_with_entities = true;
        //       // set_obj(val, FUEL);
        //     }  else if (i < 3 * chunk_size) {
        //       auto e = spawn_entity_at_idx(val, .5, RESOURCE_RED);
        //       e->collides_with_entities = true;
        //       // set_obj(val, RESOURCE_RED);
        //     } else {
        //       auto e = spawn_entity_at_idx(val, .5, RESOURCE_GREEN);
        //       e->collides_with_entities = true;
        //       // set_obj(val, RESOURCE_GREEN);
        //
        //       // std::cout << "+RGREEN: (" <<val << "," << e->x << "," << e->y << "): " << get_obj(val) << std::endl;
        //         // auto e = spawn_entity_at_idx(val, .5, ENEMY);
        //         // float vel = (.1 * rand_gen.rand01() + .1) * (rand_gen.randn(2) * 2 - 1);
        //         // if (rand_gen.rand01() < .5) {
        //         //     e->vx = vel;
        //         // } else {
        //         //     e->vy = vel;
        //         // }
        //         // e->smart_step = true;
        //         // e->collides_with_entities = true;
        //     }
        // }

        // for (int i = 0; i < num_objs; i++) {
        //   free_cells.erase(free_cells.begin()+obj_idxs[i]);
        // }

        // for (int i = 0; i < grid_size; i++) {
        //     int val = get_obj(i);
        //     // if (val == MARKER)
        //         // val = SPACE;
        //     set_obj(i, val);
        // }

        // visibility = options.distribution_mode == EasyMode ? 10 : 16;
        // visibility = 10;
    }

    void set_action_xy(int move_act) override {
        float acceleration = move_act % 3 - 1;
        if (acceleration < 0){
            acceleration *= 0.33f;
            // tank--;
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

        counter++;


        if (counter % 10 == 0) {
          std::cout << "agent: " << agent->x << "," << agent->y << " : " << fuel_container->get_value() << std::endl;
        }

        if (special_action == 1) {
            float theta = -1 * agent->rotation + PI / 2;
            float vx = cos(theta);
            float vy = sin(theta);
            auto new_bullet = add_entity_rxy(agent->x, agent->y, vx, vy, 0.1f, 0.25f, PLAYER_BULLET);
            new_bullet->expire_time = 10;
            new_bullet->rotation = agent->rotation;
        }

        for (int ent_idx = (int)(entities.size()) - 1; ent_idx >= 0; ent_idx--) {
            auto ent = entities[ent_idx];
            if (ent->type == ENEMY) {
                ent->face_direction(ent->vx, ent->vy, -1 * PI / 2);
            }

            if (ent->type != PLAYER_BULLET)
                continue;

            bool found_wall = false;

            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    int type2 = get_obj_from_floats(ent->x + ent->rx * (2 * i - 1), ent->y + ent->ry * (2 * j - 1));
                    found_wall = found_wall || type2 == CAVEWALL;
                }
            }

            if (found_wall) {
                ent->will_erase = true;
                spawn_child(ent, EXPLOSION, .5 * ent->rx);
            }
        }

        erase_if_needed();
    }
};

REGISTER_GAME("collector", Collector);
