#include <stdlib.h>
#include <ncurses.h>

#include "string.h"

#include "dungeon.h"
#include "pc.h"
#include "utils.h"
#include "move.h"
#include "path.h"

void delete_pc(character *the_pc)
{
  int i;
  for(i = 0; i <12; i++)
  {
    if(((pc*) the_pc)->equipment[i] != NULL)
    {
        delete(((pc*) the_pc)->equipment[i]);
        ((pc*) the_pc)->equipment[i] = NULL;
    }
    if(i < 10 && ((pc*) the_pc)->inventory[i] != NULL)
    {
        delete(((pc*) the_pc)->inventory[i]);
        ((pc*) the_pc)->inventory[i] = NULL;
    }
  }
  delete static_cast<pc *>(the_pc);
}
uint32_t pc_is_alive(dungeon_t *d)
{
  return ((pc *) d->pc)->alive;
}

void place_pc(dungeon_t *d)
{
  ((pc *) d->pc)->position[dim_y] = rand_range(d->rooms->position[dim_y],
                                               (d->rooms->position[dim_y] +
                                                d->rooms->size[dim_y] - 1));
  ((pc *) d->pc)->position[dim_x] = rand_range(d->rooms->position[dim_x],
                                               (d->rooms->position[dim_x] +
                                                d->rooms->size[dim_x] - 1));

  pc_init_known_terrain(d->pc);
  pc_observe_terrain(d->pc, d);
}

void config_pc(dungeon_t *d)
{
  int i;
  /* This should be in the PC constructor, now. */
  pc *the_pc;
  static dice pc_dice(0, 1, 4);

  the_pc = new pc;
  d->pc = (character *) the_pc;
  the_pc->symbol = '@';

  place_pc(d);

  the_pc->speed = PC_SPEED;
  the_pc->next_turn = 0;
  the_pc->alive = 1;
  the_pc->sequence_number = 0;
  the_pc->color.push_back(COLOR_WHITE);
  the_pc->damage = &pc_dice;
  the_pc->hp = 100;
  for(i = 0; i < 12; i++)
  {
    the_pc->equipment[i] = NULL;
    if(i < 10)
    {
        the_pc->inventory[i] = NULL;
    }
  }
    the_pc->name = "Isabella Garcia-Shapiro";

  d->charmap[the_pc->position[dim_y]]
            [the_pc->position[dim_x]] = (character *) d->pc;

  dijkstra(d);
  dijkstra_tunnel(d);
}

uint32_t pc_next_pos(dungeon_t *d, pair_t dir)
{
  dir[dim_y] = dir[dim_x] = 0;

  /* Tunnel to the nearest dungeon corner, then move around in hopes *
   * of killing a couple of monsters before we die ourself.          */

  if (in_corner(d, d->pc)) {
    /*
    dir[dim_x] = (mapxy(d->pc.position[dim_x] - 1,
                        d->pc.position[dim_y]) ==
                  ter_wall_immutable) ? 1 : -1;
    */
    dir[dim_y] = (mapxy(((pc *) d->pc)->position[dim_x],
                        ((pc *) d->pc)->position[dim_y] - 1) ==
                  ter_wall_immutable) ? 1 : -1;
  } else {
    dir_nearest_wall(d, d->pc, dir);
  }

  return 0;
}

void pc_learn_terrain(character *the_pc, pair_t pos, terrain_type_t ter)
{
  ((pc *) the_pc)->known_terrain[pos[dim_y]][pos[dim_x]] = ter;
  ((pc *) the_pc)->visible[pos[dim_y]][pos[dim_x]] = 1;
}

void pc_see_object(character *the_pc, object *o)
{
  if (o) {
    o->has_been_seen();
  }
}

void pc_reset_visibility(character *the_pc)
{
  uint32_t y, x;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      ((pc *) the_pc)->visible[y][x] = 0;
    }
  }
}

terrain_type_t pc_learned_terrain(character *the_pc, int8_t y, int8_t x)
{
  return ((pc *) the_pc)->known_terrain[y][x];
}

void pc_init_known_terrain(character *the_pc)
{
  uint32_t y, x;

  for (y = 0; y < DUNGEON_Y; y++) {
    for (x = 0; x < DUNGEON_X; x++) {
      ((pc *) the_pc)->known_terrain[y][x] = ter_unknown;
      ((pc *) the_pc)->visible[y][x] = 0;
    }
  }
}

void pc_observe_terrain(character *the_pc, dungeon_t *d)
{
  pair_t where;
  pc *p;
  int8_t y_min, y_max, x_min, x_max;

  p = (pc *) the_pc;

  y_min = p->position[dim_y] - PC_VISUAL_RANGE;
  if (y_min < 0) {
    y_min = 0;
  }
  y_max = p->position[dim_y] + PC_VISUAL_RANGE;
  if (y_max > DUNGEON_Y - 1) {
    y_max = DUNGEON_Y - 1;
  }
  x_min = p->position[dim_x] - PC_VISUAL_RANGE;
  if (x_min < 0) {
    x_min = 0;
  }
  x_max = p->position[dim_x] + PC_VISUAL_RANGE;
  if (x_max > DUNGEON_X - 1) {
    x_max = DUNGEON_X - 1;
  }

  for (where[dim_y] = y_min; where[dim_y] <= y_max; where[dim_y]++) {
    where[dim_x] = x_min;
    can_see(d, p->position, where, 1);
    where[dim_x] = x_max;
    can_see(d, p->position, where, 1);
  }
  /* Take one off the x range because we alreay hit the corners above. */
  for (where[dim_x] = x_min - 1; where[dim_x] <= x_max - 1; where[dim_x]++) {
    where[dim_y] = y_min;
    can_see(d, p->position, where, 1);
    where[dim_y] = y_max;
    can_see(d, p->position, where, 1);
  }
}

int32_t is_illuminated(character *the_pc, int8_t y, int8_t x)
{
  return ((pc *) the_pc)->visible[y][x];
}

int pc_get_free_inv_slot(character *the_pc)
{
    int i;
    for(i = 0; i < 10; i++)
    {
        if(!((pc *) the_pc)->inventory[i])
        {
            return i;
        }
    }
    return -1;
}

bool pc_pickup_object(dungeon_t *d)
{
    int iSlot;
    iSlot = pc_get_free_inv_slot((character *) d->pc);
    if(iSlot == -1)
    {
        return false;
    }

    ((pc*) d->pc)->inventory[iSlot] = d->objmap[d->pc->position[dim_y]][d->pc->position[dim_x]];
    d->objmap[d->pc->position[dim_y]][d->pc->position[dim_x]] = ((pc *) d->pc)->inventory[iSlot]->get_next();
    ((pc*) d->pc)->inventory[iSlot]->set_next(NULL);
    return true;
}

bool pc_equip(dungeon_t *d, int slot)
{
   // io_queue_message("%d", "item to be equipped: ")
   // io_display(d);

    if(slot < 0 || slot > 9)
    {
        return false;
    }
    if(((pc *)d->pc)->inventory[slot] == NULL)
    {
        return false;
    }

    int type = (((pc *)d->pc)->inventory[slot]->get_type() - 1);

    if(((pc *)d->pc)->inventory[slot]->get_type() == objtype_RING && ((pc *)d->pc)->equipment[10] != NULL)
    {
        type = 11;
    }

    if(((pc *)d->pc)->equipment[type] != NULL)
    {
        object* obj = ((pc *)d->pc)->equipment[type];
        ((pc *)d->pc)->equipment[type] = ((pc *)d->pc)->inventory[slot];
        ((pc *)d->pc)->inventory[slot] = obj;
    }
    else
    {
        ((pc *)d->pc)->equipment[type] = ((pc *)d->pc)->inventory[slot];
        ((pc *)d->pc)->inventory[slot] = NULL;
    }
     d->pc->speed += ((pc *)d->pc)->equipment[type]->get_speed();
return true;
}

bool pc_drop(dungeon_t *d, int slot, int inEquipment)
{
    if(slot < 0 || slot > 9)
    {
        return false;
    }
    if(inEquipment == 1 && ((pc *) d->pc)->equipment[slot] == NULL)
    {
        return false;
    }
    if(inEquipment == 0 && ((pc *) d->pc)->inventory[slot] == NULL)
    {
        return false;
     }

    object* obj;
    if(inEquipment == 1)
    {
       obj = ((pc *) d->pc)->equipment[slot];
       ((pc *) d->pc)->equipment[slot] = NULL;
    }
    else
    {
      obj = ((pc *) d->pc)->inventory[slot];
      ((pc *) d->pc)->inventory[slot] = NULL;
    }
    obj->set_next(d->objmap[d->pc->position[dim_y]][d->pc->position[dim_x]]);
    d->objmap[d->pc->position[dim_y]][d->pc->position[dim_x]] = obj;
    return true;
}

bool pc_takeoff(dungeon_t* d,int slot)
{
    int freeSlot = pc_get_free_inv_slot(d->pc);
    d->pc->speed -= ((pc *)d->pc)->equipment[slot]->get_speed();
    if(freeSlot == -1)
    {
        pc_drop(d,slot,1);
    }
    else
    {
        ((pc*)d->pc)->inventory[freeSlot] = ((pc*)d->pc)->equipment[slot];
        ((pc*)d->pc)->equipment[slot] = NULL;
        return true;
    }
        return true;
}

bool pc_expunge(dungeon_t* d, int slot)
{
    if(((pc *) d->pc)->inventory[slot] == NULL)
    {
        return false;
    }
    delete(((pc *) d->pc)->inventory[slot]);
    ((pc *) d->pc)->inventory[slot] = NULL;
    return true;
}

int pc_calculate_dmg(character *the_pc)
{
    int i;
    uint32_t dmg;
    dmg = 0;
    for(i = 0; i < 12; i++)
    {
        if(((pc *)the_pc)->equipment[i] != NULL)
        {
            dmg += ((pc *)the_pc)->equipment[i]->roll_dice();
        }
        else if (i == 0)
        {
            dmg += the_pc->damage->roll();
        }
    }
    return dmg;
}
