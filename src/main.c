#include <raylib.h>
#include <raymath.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

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

typedef struct Cell Cell;
struct Cell
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

void Cell_draw(Cell const * const self)
{
	Rectangle rect = Rect_CreateCentered(self->pos, self->size);
	Rectangle border = Rect_CreateCentered(self->pos, Vector2AddValue(self->size, 8.0));
	DrawRectangleRec(border, DARKGRAY);
	DrawRectangleRec(rect, self->color);
}

Rectangle Cell_getCollisionRect(Cell const * const self)
{
	return Rect_CreateCentered(self->pos, self->size);
}

typedef struct Cell_Selectable Cell_Selectable;
struct Cell_Selectable
{
	Cell as_Cell;
	bool def;
	bool owned;
	bool selected;
	Color selected_color;
	Color normal_color;
	Color owned_color;
	Cell const * attached_cell;
	Vector2 attached_pos_diff;
	size_t idx;
	char * connections;
};

void Cell_Selectable_ConnectCells(Cell_Selectable * const a, Cell_Selectable * const b)
{
	BitArray_set(a->connections, b->idx);
	BitArray_set(b->connections, a->idx);
}

void Cell_Selectable_DisconnectCells(Cell_Selectable * const a, Cell_Selectable * const b)
{
	BitArray_unset(a->connections, b->idx);
	BitArray_unset(b->connections, a->idx);
}

bool Cell_Selectable_AreCellsConnected(Cell_Selectable const * const a, Cell_Selectable const * const b)
{
	return BitArray_test(a->connections, b->idx) && BitArray_test(b->connections, a->idx);
}

void Cell_Selectable_updateColor(Cell_Selectable * const self)
{
	self->as_Cell.color = self->selected ? self->selected_color : (self->owned ? self->owned_color : self->normal_color);
}

void Cell_Selectable_setSelected(Cell_Selectable * const self, bool selected)
{
	self->selected = selected;
	Cell_Selectable_updateColor(self);
}

void Cell_Selectable_setOwned(Cell_Selectable * const self, bool owned)
{
	self->owned = self->def || owned;
	Cell_Selectable_updateColor(self);
}

void Cell_Selectable_boundToCell(Cell_Selectable * const self, Cell const * const to)
{
	if (self->attached_cell != to)
	{
		self->attached_cell = to;
		self->attached_pos_diff = Vector2Subtract(self->as_Cell.pos, to->pos);
	}
}

void Cell_Selectable_unbound(Cell_Selectable * const self)
{
	self->attached_cell = 0;
	self->attached_pos_diff = Vector2Zero();
}

void Cell_Selectable_updatePos(Cell_Selectable * const self)
{
	if (self->attached_cell)
	{
		self->as_Cell.pos = Vector2Add(self->attached_cell->pos, self->attached_pos_diff);
	}
}

Vector2 point = (Vector2) { .x = WIN_W / 2, .y = WIN_H / 2 };
Vector2 size = (Vector2) { .x = 50.0, .y = 50.0 };

#define CELLS_COUNT 15
#define FS 2 // 8 * 2 = 16 expect(CELLS_COUNT < FS * 8)

int main(void)
{
	Cell pointer = (Cell) {.pos = point, .size = (Vector2) {.x = 10.0, .y = 10.0}, .color = BLUE};

	Cell_Selectable cells[CELLS_COUNT];

	char conns[CELLS_COUNT][FS] = {{0}};

	for (int i = 0; i < CELLS_COUNT; ++i)
	{
		cells[i] = (Cell_Selectable) {
			.as_Cell = (Cell) {.pos = Vector2Add(point, (Vector2) {(i - 7) * 65.0, 0.0}), .size = size, .color = RED},
			.selected = false,
			.def = i - 7 == 0,
			.owned = i - 7 == 0,
			.selected_color = LIME,
			.owned_color = ORANGE,
			.normal_color = RED,
			.attached_cell = 0,
			.attached_pos_diff = Vector2Zero(),
			.idx = i,
			.connections = conns[i]
		};
	}
	Cell_Selectable * connecting_cell = 0;

	InitWindow(WIN_W, WIN_H, "Symbiosis");
	HideCursor();
	SetTargetFPS(TARGET_FPS);
	while (!WindowShouldClose())
	{
		Vector2 mouse_pos = GetMousePosition();
		pointer.pos = mouse_pos;

		Cell_Selectable * selected_cell = 0;
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
						Cell_Selectable_boundToCell(selected_cell, &pointer);
						Cell_Selectable_updatePos(selected_cell);
					}
				}
			}
			else
			{
				Cell_Selectable_unbound(selected_cell);
			}

			if (connecting_cell && connecting_cell != selected_cell && (IsKeyReleased(KEY_LEFT_SHIFT) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)))
			{
				if (Cell_Selectable_AreCellsConnected(selected_cell, connecting_cell))
				{
					Cell_Selectable_DisconnectCells(selected_cell, connecting_cell);
				}
				else
				{
					if (selected_cell->owned || connecting_cell->owned)
					{
						Cell_Selectable_ConnectCells(selected_cell, connecting_cell);
					}
				}
			}
		}

		if (connecting_cell && (IsKeyReleased(KEY_LEFT_SHIFT) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)))
		{
			connecting_cell = 0;
		}

		// Update owned state
		for (int i = 0; i < CELLS_COUNT; ++i)
		{
			Cell_Selectable_setOwned(&cells[i], false);
			for (int j = 0; j < CELLS_COUNT; ++j)
			{
				if (i == j) continue;
				if (Cell_Selectable_AreCellsConnected(&cells[i], &cells[j]))
				{
					if (cells[i].owned || cells[j].owned)
					{
						Cell_Selectable_setOwned(&cells[i], true);
						Cell_Selectable_setOwned(&cells[j], true);
						break;
					}
				}
			}
		}

		// Update selected state
		for (int i = 0; i < CELLS_COUNT; ++i)
		{
			bool collision = CheckCollisionRecs(Cell_getCollisionRect(&cells[i].as_Cell), Cell_getCollisionRect(&pointer));
			if (!selected_cell)
			{
				Cell_Selectable_setSelected(&cells[i], collision);
				if (collision)
				{
					selected_cell = &cells[i];
				}
			}
			else if (i != selected_cell->idx)
			{
				Cell_Selectable_setSelected(&cells[i], false);
			}
			else // if (selected_cell && i == selected_cell->idx)
			{
				Cell_Selectable_setSelected(&cells[i], collision);
				if (!collision)
				{
					selected_cell = 0;
				}
			}
		}

		BeginDrawing();
		{
			ClearBackground(LIGHTGRAY);

			// Draw connections & moving cells over them
			for (int i = 0; i < CELLS_COUNT; ++i)
			{
				for (int j = 0; j < CELLS_COUNT; ++j)
				{
					if (i != j && Cell_Selectable_AreCellsConnected(&cells[i], &cells[j]))
					{
						DrawLineV(cells[i].as_Cell.pos, cells[j].as_Cell.pos, BLACK);
						Vector2 diff = Vector2Subtract(cells[j].as_Cell.pos, cells[i].as_Cell.pos);
						Vector2 direction = Vector2Normalize(diff);
						float length = Vector2Length(diff);
						float speed = 50.0;
						Vector2 pixel_pos = Vector2Add(cells[i].as_Cell.pos, Vector2Scale(direction, fmod(GetTime() * speed, length)));
						DrawRectangleRec(Rect_CreateCentered(pixel_pos, (Vector2) {.x = 5, .y = 5}), BLACK);
					}
				}
			}

			// Draw cells
			{
				for (int i = 0; i < CELLS_COUNT; ++i)
				{
					if (!selected_cell || (selected_cell && selected_cell->idx != i))
					{
						Cell_draw(&cells[i].as_Cell);
					}
				}

				// Draw selected cells over others
				if (selected_cell)
				{
					Cell_draw(&selected_cell->as_Cell);
				}
			}

			if (connecting_cell)
			{
				DrawLineV(connecting_cell->as_Cell.pos, pointer.pos, WHITE);
			}

			Cell_draw(&pointer);
		}
		EndDrawing();
	}
	CloseWindow();
	return 0;
}
