#ifndef __OBJECT_SHAPES_H__
#define __OBJECT_SHAPES_H__

// Unit
#define UNIT_OBJECT_SIZE					Size(24, 32)
#define UNIT_ANCHOR_POINT					Point(0.5f, -0.22f)
#define UNIT_OBJECT_RADIUS_IN_MAP			12.f
#define UNIT_LAUNCH_CENTER					Point(0, 31)
// The value will affect the order in which the unit and snowball and its shadow are occluded, 
// as well as the starting position of the snowball's throw.
#define UNIT_LAUNCH_RADIUS_IN_MAP			16.0f

// Projectile
#define PROJECTILE_OBJECT_SIZE				Size(10, 10)
#define PROJECTILE_ANCHOR_POINT				Point(0.5f, 0.5f)
#define PROJECTILE_OBJECT_RADIUS_IN_MAP		5.f

// ItemBox
#define ITEMBOX_OBJECT_SIZE					Size(32, 32)
#define ITEMBOX_ANCHOR_POINT				Point(0.5f, 0.0f)
#define ITEMBOX_OBJECT_RADIUS_IN_MAP		16.f
#define ITEMBOX_LAUNCH_CENTER				Point(0, 10)

// Item
#define ITEM_OBJECT_RADIUS_IN_MAP			16.f


#endif // __OBJECT_SHAPES_H__