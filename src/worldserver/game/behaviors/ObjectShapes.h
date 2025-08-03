#ifndef __OBJECT_SHAPES_H__
#define __OBJECT_SHAPES_H__

// 单位
#define UNIT_OBJECT_SIZE					Size(24, 32)
#define UNIT_ANCHOR_POINT					Point(0.5f, -0.22f)
#define UNIT_OBJECT_RADIUS_IN_MAP			12.f
#define UNIT_LAUNCH_CENTER					Point(0, 31)
#define UNIT_LAUNCH_RADIUS_IN_MAP			16.0f			// 值的大小将影响单位与雪球及雪球阴影的遮挡关系和雪球的投射起始位置

// 抛射体
#define PROJECTILE_OBJECT_SIZE				Size(10, 10)
#define PROJECTILE_ANCHOR_POINT				Point(0.5f, 0.5f)
#define PROJECTILE_OBJECT_RADIUS_IN_MAP		5.f

// 物品箱
#define ITEMBOX_OBJECT_SIZE					Size(32, 32)
#define ITEMBOX_ANCHOR_POINT				Point(0.5f, 0.0f)
#define ITEMBOX_OBJECT_RADIUS_IN_MAP		16.f
#define ITEMBOX_LAUNCH_CENTER				Point(0, 10)

// 物品
#define ITEM_OBJECT_RADIUS_IN_MAP			16.f


#endif // __OBJECT_SHAPES_H__