#include "game/objects/general/door.h"

#include "game/collide.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/utils.h>

#include <stdbool.h>
#include <stddef.h>

static bool Door_LaraDoorCollision(const FLOOR_INFO *floor);
static void Door_Check(DOORPOS_DATA *d);
static void Door_Open(DOORPOS_DATA *d);
static void Door_Shut(DOORPOS_DATA *d);

static bool Door_LaraDoorCollision(const FLOOR_INFO *const floor)
{
    // Check if Lara is on the same tile as the invisible block.
    if (g_LaraItem == NULL) {
        return false;
    }

    int16_t room_num = g_LaraItem->room_number;
    const FLOOR_INFO *const lara_floor = Room_GetFloor(
        g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z, &room_num);
    return lara_floor == floor;
}

static void Door_Check(DOORPOS_DATA *const d)
{
    // Forcefully remove the invisible block if Lara happens to occupy the same
    // tile. This ensures that Lara doesn't void if a timed door happens to
    // close right on her, or the player loads the game while standing on a
    // closed door's block tile.
    if (Door_LaraDoorCollision(d->floor)) {
        Door_Open(d);
    }
}

static void Door_Shut(DOORPOS_DATA *const d)
{
    // Change the level geometry so that the door tile is impassable.
    FLOOR_INFO *const floor = d->floor;
    if (floor == NULL) {
        return;
    }

    floor->index = 0;
    floor->box = NO_BOX;
    floor->floor = NO_HEIGHT / STEP_L;
    floor->ceiling = NO_HEIGHT / STEP_L;
    floor->sky_room = NO_ROOM;
    floor->pit_room = NO_ROOM;

    const int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index |= BLOCKED;
    }
}

static void Door_Open(DOORPOS_DATA *const d)
{
    // Restore the level geometry so that the door tile is passable.
    FLOOR_INFO *const floor = d->floor;
    if (!floor) {
        return;
    }

    *floor = d->old_floor;

    const int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index &= ~BLOCKED;
    }
}

void Door_Setup(OBJECT_INFO *obj)
{
    obj->initialise = Door_Initialise;
    obj->control = Door_Control;
    obj->draw_routine = Object_DrawUnclippedItem;
    obj->collision = Door_Collision;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Door_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    DOOR_DATA *door = GameBuf_Alloc(sizeof(DOOR_DATA), GBUF_EXTRA_DOOR_STUFF);
    item->data = door;

    int32_t dx = 0;
    int32_t dy = 0;
    if (item->rot.y == 0) {
        dx--;
    } else if (item->rot.y == -PHD_180) {
        dx++;
    } else if (item->rot.y == PHD_90) {
        dy--;
    } else {
        dy++;
    }

    ROOM_INFO *r;
    ROOM_INFO *b;
    int32_t x_floor;
    int32_t y_floor;
    int16_t room_num;
    int16_t box_num;

    r = &g_RoomInfo[item->room_number];
    x_floor = ((item->pos.z - r->z) >> WALL_SHIFT) + dx;
    y_floor = ((item->pos.x - r->x) >> WALL_SHIFT) + dy;
    door->d1.floor = &r->floor[x_floor + y_floor * r->x_size];
    room_num = Room_GetDoor(door->d1.floor);
    if (room_num == NO_ROOM) {
        box_num = door->d1.floor->box;
    } else {
        b = &g_RoomInfo[room_num];
        x_floor = ((item->pos.z - b->z) >> WALL_SHIFT) + dx;
        y_floor = ((item->pos.x - b->x) >> WALL_SHIFT) + dy;
        box_num = b->floor[x_floor + y_floor * b->x_size].box;
    }
    if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door->d1.block = box_num;
    door->d1.old_floor = *door->d1.floor;

    if (r->flipped_room != -1) {
        r = &g_RoomInfo[r->flipped_room];
        x_floor = ((item->pos.z - r->z) >> WALL_SHIFT) + dx;
        y_floor = ((item->pos.x - r->x) >> WALL_SHIFT) + dy;
        door->d1flip.floor = &r->floor[x_floor + y_floor * r->x_size];
        room_num = Room_GetDoor(door->d1flip.floor);
        if (room_num == NO_ROOM) {
            box_num = door->d1flip.floor->box;
        } else {
            b = &g_RoomInfo[room_num];
            x_floor = ((item->pos.z - b->z) >> WALL_SHIFT) + dx;
            y_floor = ((item->pos.x - b->x) >> WALL_SHIFT) + dy;
            box_num = b->floor[x_floor + y_floor * b->x_size].box;
        }
        if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
            box_num = NO_BOX;
        }
        door->d1flip.block = box_num;
        door->d1flip.old_floor = *door->d1flip.floor;
    } else {
        door->d1flip.floor = NULL;
    }

    room_num = Room_GetDoor(door->d1.floor);
    Door_Shut(&door->d1);
    Door_Shut(&door->d1flip);

    if (room_num == NO_ROOM) {
        door->d2.floor = NULL;
        door->d2flip.floor = NULL;
        return;
    }

    r = &g_RoomInfo[room_num];
    x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    door->d2.floor = &r->floor[x_floor + y_floor * r->x_size];
    room_num = Room_GetDoor(door->d2.floor);
    if (room_num == NO_ROOM) {
        box_num = door->d2.floor->box;
    } else {
        b = &g_RoomInfo[room_num];
        x_floor = (item->pos.z - b->z) >> WALL_SHIFT;
        y_floor = (item->pos.x - b->x) >> WALL_SHIFT;
        box_num = b->floor[x_floor + y_floor * b->x_size].box;
    }
    if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
        box_num = NO_BOX;
    }
    door->d2.block = box_num;
    door->d2.old_floor = *door->d2.floor;

    if (r->flipped_room != -1) {
        r = &g_RoomInfo[r->flipped_room];
        x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
        y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
        door->d2flip.floor = &r->floor[x_floor + y_floor * r->x_size];
        room_num = Room_GetDoor(door->d2flip.floor);
        if (room_num == NO_ROOM) {
            box_num = door->d2flip.floor->box;
        } else {
            b = &g_RoomInfo[room_num];
            x_floor = (item->pos.z - b->z) >> WALL_SHIFT;
            y_floor = (item->pos.x - b->x) >> WALL_SHIFT;
            box_num = b->floor[x_floor + y_floor * b->x_size].box;
        }
        if (!(g_Boxes[box_num].overlap_index & BLOCKABLE)) {
            box_num = NO_BOX;
        }
        door->d2flip.block = box_num;
        door->d2flip.old_floor = *door->d2flip.floor;
    } else {
        door->d2flip.floor = NULL;
    }

    Door_Shut(&door->d2);
    Door_Shut(&door->d2flip);
}

void Door_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    DOOR_DATA *door = item->data;

    if (Item_IsTriggerActive(item)) {
        if (item->current_anim_state == DOOR_CLOSED) {
            item->goal_anim_state = DOOR_OPEN;
        } else {
            Door_Open(&door->d1);
            Door_Open(&door->d2);
            Door_Open(&door->d1flip);
            Door_Open(&door->d2flip);
        }
    } else {
        if (item->current_anim_state == DOOR_OPEN) {
            item->goal_anim_state = DOOR_CLOSED;
        } else {
            Door_Shut(&door->d1);
            Door_Shut(&door->d2);
            Door_Shut(&door->d1flip);
            Door_Shut(&door->d2flip);
        }
    }

    Door_Check(&door->d1);
    Door_Check(&door->d2);
    Door_Check(&door->d1flip);
    Door_Check(&door->d2flip);
    Item_Animate(item);
}

void Door_Collision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        if (item->current_anim_state != item->goal_anim_state) {
            Lara_Push(item, coll, coll->enable_spaz, true);
        } else {
            Lara_Push(item, coll, false, true);
        }
    }
}
