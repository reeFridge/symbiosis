#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>

#define CELLS_COUNT 15
#define FS 2 // 8 * 2 = 16 expect(CELLS_COUNT < FS * 8)
#define TICK_SEC 5
#define WIN_W 1024
#define WIN_H 768
#define TARGET_FPS 60

void BitArray_set(char* arr, unsigned int idx)
{
	size_t block_idx = idx / CHAR_BIT;
	char bit_offset = idx % CHAR_BIT;
	char mask = 1;
	mask = mask << bit_offset;
	arr[block_idx] |= mask;
}

void BitArray_unset(char* arr, unsigned int idx)
{
	size_t block_idx = idx / CHAR_BIT;
	char bit_offset = idx % CHAR_BIT;
	char mask = 1;
	mask = mask << bit_offset;
	arr[block_idx] &= ~mask;
}

bool BitArray_test(char* arr, unsigned int idx)
{
	size_t block_idx = idx / CHAR_BIT;
	char bit_offset = idx % CHAR_BIT;
	char mask = 1;
	mask = mask << bit_offset;
	
	return !!(arr[block_idx] & mask);
}

unsigned int BitArray_count(char* arr)
{
	unsigned int res = 0;
	for (int i = 0; i < CELLS_COUNT; ++i)
	{
		if (BitArray_test(arr, i))
		{
			++res;
		}
	}

	return res;
}

typedef struct Box Box;
struct Box
{
	Vector2 pos;
	Vector2 size;
	Color color;
};

Rectangle Rect_CreateCentered(Vector2 const pos, Vector2 const size)
{
	Rectangle rect = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };
	return rect;
}

void Box_draw(Box const * const self)
{
	Rectangle rect = Rect_CreateCentered(self->pos, self->size);
	Rectangle border = Rect_CreateCentered(self->pos, Vector2AddValue(self->size, 8.0));
	DrawRectangleRec(border, DARKGRAY);
	DrawRectangleRec(rect, self->color);
}

Rectangle Box_getCollisionRect(Box const * const self)
{
	return Rect_CreateCentered(self->pos, self->size);
}

typedef enum Cell_State Cell_State;
enum Cell_State {ALIVE, DEAD};

typedef struct Cell Cell;
struct Cell
{
	Box as_Box;
	bool def;
	bool owned;
	bool selected;
	Color selected_color;
	Color normal_color;
	Color owned_color;
	Cell_State state;
	int res;
	int const_out;
	Box const * attached_box;
	Vector2 attached_pos_diff;
	size_t idx;
	char * connections;
};

void Cell_ConnectCells(Cell * const a, Cell * const b)
{
	BitArray_set(a->connections, b->idx);
	BitArray_set(b->connections, a->idx);
}

void Cell_DisconnectCells(Cell * const a, Cell * const b)
{
	BitArray_unset(a->connections, b->idx);
	BitArray_unset(b->connections, a->idx);
}

bool Cell_AreCellsConnected(Cell const * const a, Cell const * const b)
{
	return BitArray_test(a->connections, b->idx) && BitArray_test(b->connections, a->idx);
}

void Cell_updateColor(Cell * const self)
{
	switch (self->state)
	{
		case ALIVE:
			self->as_Box.color = self->selected
				? self->selected_color
				: (self->owned
						? (self->def
							? PURPLE
							: self->owned_color)
						: self->normal_color);
			break;
		case DEAD:
			self->as_Box.color = DARKGRAY;
			break;
		default:
			break;
	}
}

void Cell_setSelected(Cell * const self, bool selected)
{
	self->selected = selected;
	Cell_updateColor(self);
}

void Cell_setOwned(Cell * const self, bool owned)
{
	self->owned = self->def || owned;
	Cell_updateColor(self);
}

void Cell_boundToBox(Cell * const self, Box const * const box, Camera2D camera)
{
	if (self->attached_box != box)
	{
		self->attached_box = box;
		self->attached_pos_diff = Vector2Subtract(self->as_Box.pos, GetScreenToWorld2D(box->pos, camera));
	}
}

unsigned int Cell_getOveralOut(Cell const * const self)
{
	const unsigned int connections_count = BitArray_count(self->connections);
	return self->const_out * connections_count;
}

void Cell_exchangeRes(Cell * const self, Cell const * const in)
{
	self->res += Cell_getOveralOut(in) - Cell_getOveralOut(self);
}

void Cell_updateState(Cell * const self)
{
	switch (self->state)
	{
		case ALIVE:
			break;
		default:
			break;
	}

	if (self->res <= 0)
	{
		self->state = DEAD;
	}
}

void Cell_unbound(Cell * const self)
{
	self->attached_box = 0;
	self->attached_pos_diff = Vector2Zero();
}

void Cell_updatePos(Cell * const self, Camera2D camera)
{
	if (self->attached_box)
	{
		self->as_Box.pos = Vector2Add(GetScreenToWorld2D(self->attached_box->pos, camera), self->attached_pos_diff);
	}
}

Vector2 point = (Vector2) { .x = WIN_W / 2, .y = WIN_H / 2 };
Vector2 size = (Vector2) { .x = 50.0, .y = 50.0 };

// rules
// cell.out = f[m](c)
// where: m - mode, c - connections_count
// cell.res = cell[c].out - cell.out
// cell[c].res = cell.out - cell[c].out
// cell.state = {ALIVE, DEAD}
// tick = 1000ms
// foreach tick update_state(&cell)

int main(void)
{
	Box pointer = (Box) {.pos = point, .size = (Vector2) {.x = 10.0, .y = 10.0}, .color = BLUE};

	Cell cells[CELLS_COUNT];
	Camera2D camera = {0};
	camera.zoom = 1.0f;
    camera.offset = (Vector2) { WIN_W / 2, WIN_H / 2 };
    camera.rotation = 0.0f;
	camera.target = point;
	unsigned int tick = 0;
	float last_tick_time = 0.0f;

	char conns[CELLS_COUNT][FS] = {{0}};

	for (int i = 0; i < CELLS_COUNT; ++i)
	{
		cells[i] = (Cell) {
			.as_Box = (Box) {.pos = Vector2Add(point, (Vector2) {(i - 7) * 65.0, 0.0}), .size = size, .color = RED},
			.res = 10,
			.const_out = 1,
			.state = ALIVE,
			.selected = false,
			.def = i - 7 == 0,
			.owned = i - 7 == 0,
			.selected_color = LIME,
			.owned_color = ORANGE,
			.normal_color = RED,
			.attached_box = 0,
			.attached_pos_diff = Vector2Zero(),
			.idx = i,
			.connections = conns[i]
		};
	}
	Cell * connecting_cell = 0;

	InitWindow(WIN_W, WIN_H, "Symbiosis");
	HideCursor();
	SetTargetFPS(TARGET_FPS);
	while (!WindowShouldClose())
	{
		float time = GetTime();
		if (time - last_tick_time > TICK_SEC)
		{
			++tick;
			last_tick_time = time;
			for (int i = 0; i < CELLS_COUNT; ++i)
			{
				for (int j = 0; j < CELLS_COUNT; ++j)
				{
					if (i == j) continue;
					if (Cell_AreCellsConnected(&cells[i], &cells[j]))
					{
						Cell_exchangeRes(&cells[i], &cells[j]);
					}
				}

				Cell_updateState(&cells[i]);
			}
		}

		// Update owned state
		for (int i = 0; i < CELLS_COUNT; ++i)
		{
			Cell_setOwned(&cells[i], false);
			for (int j = 0; j < CELLS_COUNT; ++j)
			{
				if (i == j) continue;
				if (Cell_AreCellsConnected(&cells[i], &cells[j]))
				{
					if (cells[j].state == DEAD || cells[i].state == DEAD)
					{
						Cell_DisconnectCells(&cells[i], &cells[j]);
					}

					if (cells[j].owned)
					{
						Cell_setOwned(&cells[i], true);
						Cell_setOwned(&cells[j], true);
						break;
					}
				}
			}
		}

		Vector2 mouse_pos = GetMousePosition();
		pointer.pos = mouse_pos;

		Cell * selected_cell = 0;
		for (int i = 0; i < CELLS_COUNT; ++i)
		{
			if (cells[i].selected)
			{
				selected_cell = &cells[i];
				break;
			}
		}

		if (selected_cell) {
			if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
			{
				if (IsKeyDown(KEY_LEFT_SHIFT))
				{
					if (!connecting_cell)
					{
						connecting_cell = selected_cell;
					}
				}
				else
				{
					if (selected_cell->owned)
					{
						Cell_boundToBox(selected_cell, &pointer, camera);
						Cell_updatePos(selected_cell, camera);
					}
				}
			}
			else
			{
				Cell_unbound(selected_cell);
			}

			if (
					connecting_cell && connecting_cell->state == ALIVE &&
					connecting_cell != selected_cell && selected_cell->state == ALIVE &&
					(IsKeyReleased(KEY_LEFT_SHIFT) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
			   )
			{
				if (Cell_AreCellsConnected(selected_cell, connecting_cell))
				{
					Cell_DisconnectCells(selected_cell, connecting_cell);
				}
				else
				{
					if (selected_cell->owned || connecting_cell->owned)
					{
						Cell_ConnectCells(selected_cell, connecting_cell);
					}
				}
			}
		}

		if (connecting_cell && (IsKeyReleased(KEY_LEFT_SHIFT) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)))
		{
			connecting_cell = 0;
		}

		// Update selected state
		for (int i = 0; i < CELLS_COUNT; ++i)
		{
			Box worldPointer = (Box) { .pos = GetScreenToWorld2D(pointer.pos, camera), .size = pointer.size, .color = pointer.color };
			bool collision = CheckCollisionRecs(Box_getCollisionRect(&cells[i].as_Box), Box_getCollisionRect(&worldPointer));
			if (!selected_cell)
			{
				Cell_setSelected(&cells[i], collision);
				if (collision)
				{
					selected_cell = &cells[i];
				}
			}
			else if (i != selected_cell->idx)
			{
				Cell_setSelected(&cells[i], false);
			}
			else // if (selected_cell && i == selected_cell->idx)
			{
				Cell_setSelected(&cells[i], collision);
				if (!collision)
				{
					selected_cell = 0;
				}
			}
		}

        // Camera controls
        camera.zoom = Clamp(camera.zoom + ((float)GetMouseWheelMove()*0.05f), 0.5f, 1.0f);

		Vector2 direction = Vector2Zero();
		const float velocity = 3 * 100.0f;
		if (IsKeyDown(KEY_RIGHT))
		{
			direction = Vector2Add(direction, (Vector2) { 1.0f, 0.0f });
		}
		else if (IsKeyDown(KEY_LEFT))
		{
			direction = Vector2Add(direction, (Vector2) { -1.0f, 0.0f });
		}

		if (IsKeyDown(KEY_DOWN))
		{
			direction = Vector2Add(direction, (Vector2) { 0.0f, 1.0f });
		}
		else if (IsKeyDown(KEY_UP))
		{
			direction = Vector2Add(direction, (Vector2) { 0.0f, -1.0f });
		}

		if (Vector2Length(direction) > 0.0f)
		{
			direction = Vector2Scale(Vector2Normalize(direction), velocity * GetFrameTime());
		}
		camera.target = Vector2Add(camera.target, direction);

		BeginDrawing();
		{
			ClearBackground(LIGHTGRAY);

			BeginMode2D(camera);
			{
				// Draw connections & moving cells over them
				for (int i = 0; i < CELLS_COUNT; ++i)
				{
					for (int j = 0; j < CELLS_COUNT; ++j)
					{
						if (i != j && Cell_AreCellsConnected(&cells[i], &cells[j]))
						{
							DrawLineV(cells[i].as_Box.pos, cells[j].as_Box.pos, BLACK);
							Vector2 diff = Vector2Subtract(cells[j].as_Box.pos, cells[i].as_Box.pos);
							Vector2 direction = Vector2Normalize(diff);
							float length = Vector2Length(diff);
							float speed = 50.0;
							Vector2 pixel_pos = Vector2Add(cells[i].as_Box.pos, Vector2Scale(direction, fmod(GetTime() * speed, length)));
							DrawRectangleRec(Rect_CreateCentered(pixel_pos, (Vector2) {.x = 5, .y = 5}), BLACK);
							
							const char * fmt = "%d\0";
							const unsigned int out = Cell_getOveralOut(&cells[i]);
							int size = snprintf(0, 0, fmt, out);
							char buffer[size + 1];
							snprintf(buffer, sizeof buffer, fmt, out);
							DrawText(buffer, pixel_pos.x - 20, pixel_pos.y, 10, BLACK);
						}
					}
				}

				// Draw cells
				{
					for (int i = 0; i < CELLS_COUNT; ++i)
					{
						if (!selected_cell || (selected_cell && selected_cell->idx != i))
						{
							Box_draw(&cells[i].as_Box);
						}
					}

					for (int i = 0; i < CELLS_COUNT; ++i)
					{
						const char * fmt = "%d\0";
						int size = snprintf(0, 0, fmt, cells[i].res);
						char buffer[size + 1];
						snprintf(buffer, sizeof buffer, fmt, cells[i].res);
						Vector2 pos = cells[i].as_Box.pos;
						int width = MeasureText(buffer, 20);
						DrawText(buffer, pos.x - width / 2, pos.y - 50, 20, BLACK);
					}

					// Draw selected cells over others
					if (selected_cell)
					{
						Box_draw(&selected_cell->as_Box);
					}
				}

				if (connecting_cell)
				{
					DrawLineV(connecting_cell->as_Box.pos, GetScreenToWorld2D(pointer.pos, camera), WHITE);
				}
			}
			EndMode2D();

			const char * fmt = "tick: %d\0";
			int size = snprintf(0, 0, fmt, tick);
			char buffer[size + 1];
			snprintf(buffer, sizeof buffer, fmt, tick);
			DrawText(buffer, 10, 10, 20, DARKGRAY);
			Box_draw(&pointer);
		}
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
