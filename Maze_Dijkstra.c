#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void Maze_Mapping();
void Maze_setting();
void Dijk_Initialization();

#define DIJK_MAXCOST 999
#define DIJK_WALLCOST 1000
#define MAX_QUEUE_NUM 1000

#define MAX_HEAP_NUM 2000

#define ST_COST 5
#define DIAG_COST 4
#define CON_COST 2

void Shortest_Pass_Compression();
void Shortest_Pass_Compression_NANAME();
void Pass_zero_act();

int G_Gool_X = 7;
int G_Gool_Y = 7;

int G_Maze_Row[17];
int G_Maze_Column[17];

int G_Robot_Direction = 0;
int G_Short_Pass[255] = {0};

typedef struct
{
	int cost;
	int x;
	int y;
	int direction;
	int isRow;
	int isConfirm;
} NODE_T;

typedef struct
{
	int head;
	int tail;
	NODE_T *data[MAX_QUEUE_NUM];
} Queue_T;

NODE_T node_Row[17][17];
NODE_T node_Column[17][17];

int G_Short_Pass_CP[255];
int G_Short_Pass_NANAME[255];

int NANAME_Flag = 0;

int i, j;

// typedef struct{
// 	int head;
// 	int tail;
// 	int  data[MAX_QUEUE_NUM];
// }Queue_T_adachi;

void pushQueue_walk_node(Queue_T *queue, NODE_T *input)
{
	/* データをデータの最後尾の１つ後ろに格納*/
	queue->data[queue->tail] = input;
	/* データの最後尾を１つ後ろに移動*/
	queue->tail = queue->tail + 1;
	/* 巡回シフト*/
	if (queue->tail == MAX_QUEUE_NUM)
		queue->tail = 0;
	/* スタックが満杯なら何もせず関数終了*/
	if (queue->tail == queue->head)
	{
		// printf("queue_full\n");return;
	}
}

NODE_T *popqueue_walk_node(Queue_T *queue)
{
	NODE_T *ret = NULL;
	/* スタックが空なら何もせずに関数終了*/
	if (queue->tail == queue->head)
	{
		// printf("queue_empty\n");
		//  ret->cost=65535;
		return ret;
	}
	/* データの最前列からデータを取得*/
	ret = queue->data[queue->head];
	/* データの最前列を１つ前にずらす*/
	queue->head = queue->head + 1;
	/* 巡回シフト*/
	if (queue->head == MAX_QUEUE_NUM)
		queue->head = 0;
	/* 取得したデータを返却*/
	return ret;
}

typedef struct
{
	NODE_T *data[MAX_HEAP_NUM];
	int size;
} MinHeap_T;

void swapNode(NODE_T **a, NODE_T **b)
{
	NODE_T *temp = *a;
	*a = *b;
	*b = temp;
}

void pushHeap(MinHeap_T *heap, NODE_T *node)
{
	if (heap->size >= MAX_HEAP_NUM)
		return;
	int i = heap->size;
	heap->data[i] = node;
	heap->size++;

	// 上方向へのヒープ再構築（コストが小さいものを上に）
	while (i != 0 && heap->data[(i - 1) / 2]->cost > heap->data[i]->cost)
	{
		swapNode(&heap->data[i], &heap->data[(i - 1) / 2]);
		i = (i - 1) / 2;
	}
}

NODE_T *popHeap(MinHeap_T *heap)
{
	if (heap->size <= 0)
		return NULL;
	if (heap->size == 1)
	{
		heap->size--;
		return heap->data[0];
	}

	NODE_T *root = heap->data[0];
	heap->data[0] = heap->data[heap->size - 1];
	heap->size--;

	int i = 0;
	// 下方向へのヒープ再構築
	while (1)
	{
		int left = 2 * i + 1;
		int right = 2 * i + 2;
		int smallest = i;

		if (left < heap->size && heap->data[left]->cost < heap->data[smallest]->cost)
			smallest = left;
		if (right < heap->size && heap->data[right]->cost < heap->data[smallest]->cost)
			smallest = right;

		if (smallest != i)
		{
			swapNode(&heap->data[i], &heap->data[smallest]);
			i = smallest;
		}
		else
		{
			break;
		}
	}
	return root;
}

int main()
{
	Maze_setting();

	Queue_T queue_node;
	queue_node.head = 0;
	queue_node.tail = 0;

	unsigned short Row_X;
	unsigned short Row_Y;
	unsigned short Column_X;
	unsigned short Column_Y;

	for (int i = 0; i < 17; i++)
	{ // dijk 初期化
		for (int j = 0; j < 17; j++)
		{
			node_Row[i][j].cost = DIJK_MAXCOST;
			node_Row[i][j].isRow = 1;
			node_Row[i][j].isConfirm = 0;
			node_Row[i][j].x = i;
			node_Row[i][j].y = j;
			node_Column[i][j].cost = DIJK_MAXCOST;
			node_Column[i][j].isRow = 0;
			node_Column[i][j].isConfirm = 0;
			node_Column[i][j].x = i;
			node_Column[i][j].y = j;
		}
	}

	for (int i = 0; i < 17; i++)
	{ // dijk 壁入れ
		for (int j = 0; j < 17; j++)
		{
			if ((G_Maze_Row[j] & (1 << i)) == (1 << i))
			{
				node_Row[i][j].cost = DIJK_WALLCOST;
			}
			if ((G_Maze_Column[i] & (1 << j)) == (1 << j))
			{
				node_Column[i][j].cost = DIJK_WALLCOST;
			}
		}
	}

	// --- ゴールノードの初期化とヒープ追加 ---
	MinHeap_T heap_node;
	heap_node.size = 0;

	node_Row[G_Gool_X][G_Gool_Y + 1].cost = 0;
	node_Row[G_Gool_X + 1][G_Gool_Y + 1].cost = 0;
	node_Column[G_Gool_X + 1][G_Gool_Y].cost = 0;
	node_Column[G_Gool_X + 1][G_Gool_Y + 1].cost = 0;

	pushHeap(&heap_node, &node_Row[G_Gool_X][G_Gool_Y + 1]);
	pushHeap(&heap_node, &node_Row[G_Gool_X + 1][G_Gool_Y + 1]);
	pushHeap(&heap_node, &node_Column[G_Gool_X + 1][G_Gool_Y]);
	pushHeap(&heap_node, &node_Column[G_Gool_X + 1][G_Gool_Y + 1]);

	while (1)
	{
		NODE_T* popNode = popHeap(&heap_node);
    
        if(popNode == NULL){ // キューが空になれば探索終了
            break;  
        }

        if(popNode->isConfirm == 1){
            continue;
        }
        popNode->isConfirm = 1;

		if (popNode->isRow == 1)
		{ // Rowノードからの展開
			// 北 (N: 0)
			if (node_Row[popNode->x][popNode->y + 1].cost != DIJK_WALLCOST && node_Row[popNode->x][popNode->y + 1].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 0 ? CON_COST : ST_COST);
				if (node_Row[popNode->x][popNode->y + 1].cost > next_cost)
				{
					node_Row[popNode->x][popNode->y + 1].cost = next_cost;
					node_Row[popNode->x][popNode->y + 1].direction = 0;
					pushHeap(&heap_node, &node_Row[popNode->x][popNode->y + 1]);
				}
			}
			// 南 (S: 4)
			if (node_Row[popNode->x][popNode->y - 1].cost != DIJK_WALLCOST && node_Row[popNode->x][popNode->y - 1].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 4 ? CON_COST : ST_COST);
				if (node_Row[popNode->x][popNode->y - 1].cost > next_cost)
				{
					node_Row[popNode->x][popNode->y - 1].cost = next_cost;
					node_Row[popNode->x][popNode->y - 1].direction = 4;
					pushHeap(&heap_node, &node_Row[popNode->x][popNode->y - 1]);
				}
			}
			// 北西 (NW: 7)
			if (node_Column[popNode->x][popNode->y].cost != DIJK_WALLCOST && node_Column[popNode->x][popNode->y].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 7 ? CON_COST : DIAG_COST);
				if (node_Column[popNode->x][popNode->y].cost > next_cost)
				{
					node_Column[popNode->x][popNode->y].cost = next_cost;
					node_Column[popNode->x][popNode->y].direction = 7;
					pushHeap(&heap_node, &node_Column[popNode->x][popNode->y]);
				}
			}
			// 北東 (NE: 1)
			if (node_Column[popNode->x + 1][popNode->y].cost != DIJK_WALLCOST && node_Column[popNode->x + 1][popNode->y].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 1 ? CON_COST : DIAG_COST);
				if (node_Column[popNode->x + 1][popNode->y].cost > next_cost)
				{
					node_Column[popNode->x + 1][popNode->y].cost = next_cost;
					node_Column[popNode->x + 1][popNode->y].direction = 1;
					pushHeap(&heap_node, &node_Column[popNode->x + 1][popNode->y]);
				}
			}
			// 南西 (SW: 5)
			if (node_Column[popNode->x][popNode->y - 1].cost != DIJK_WALLCOST && node_Column[popNode->x][popNode->y - 1].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 5 ? CON_COST : DIAG_COST);
				if (node_Column[popNode->x][popNode->y - 1].cost > next_cost)
				{
					node_Column[popNode->x][popNode->y - 1].cost = next_cost;
					node_Column[popNode->x][popNode->y - 1].direction = 5;
					pushHeap(&heap_node, &node_Column[popNode->x][popNode->y - 1]);
				}
			}
			// 南東 (SE: 3)
			if (node_Column[popNode->x + 1][popNode->y - 1].cost != DIJK_WALLCOST && node_Column[popNode->x + 1][popNode->y - 1].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 3 ? CON_COST : DIAG_COST);
				if (node_Column[popNode->x + 1][popNode->y - 1].cost > next_cost)
				{
					node_Column[popNode->x + 1][popNode->y - 1].cost = next_cost;
					node_Column[popNode->x + 1][popNode->y - 1].direction = 3;
					pushHeap(&heap_node, &node_Column[popNode->x + 1][popNode->y - 1]);
				}
			}
		}
		else
		{ // Columnノードからの展開
			// 東 (E: 2)
			if (node_Column[popNode->x + 1][popNode->y].cost != DIJK_WALLCOST && node_Column[popNode->x + 1][popNode->y].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 2 ? CON_COST : ST_COST);
				if (node_Column[popNode->x + 1][popNode->y].cost > next_cost)
				{
					node_Column[popNode->x + 1][popNode->y].cost = next_cost;
					node_Column[popNode->x + 1][popNode->y].direction = 2;
					pushHeap(&heap_node, &node_Column[popNode->x + 1][popNode->y]);
				}
			}
			// 西 (W: 6)
			if (node_Column[popNode->x - 1][popNode->y].cost != DIJK_WALLCOST && node_Column[popNode->x - 1][popNode->y].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 6 ? CON_COST : ST_COST);
				if (node_Column[popNode->x - 1][popNode->y].cost > next_cost)
				{
					node_Column[popNode->x - 1][popNode->y].cost = next_cost;
					node_Column[popNode->x - 1][popNode->y].direction = 6;
					pushHeap(&heap_node, &node_Column[popNode->x - 1][popNode->y]);
				}
			}
			// 北東 (NE: 1)
			if (node_Row[popNode->x][popNode->y + 1].cost != DIJK_WALLCOST && node_Row[popNode->x][popNode->y + 1].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 1 ? CON_COST : DIAG_COST);
				if (node_Row[popNode->x][popNode->y + 1].cost > next_cost)
				{
					node_Row[popNode->x][popNode->y + 1].cost = next_cost;
					node_Row[popNode->x][popNode->y + 1].direction = 1;
					pushHeap(&heap_node, &node_Row[popNode->x][popNode->y + 1]);
				}
			}
			// 南東 (SE: 3)
			if (node_Row[popNode->x][popNode->y].cost != DIJK_WALLCOST && node_Row[popNode->x][popNode->y].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 3 ? CON_COST : DIAG_COST);
				if (node_Row[popNode->x][popNode->y].cost > next_cost)
				{
					node_Row[popNode->x][popNode->y].cost = next_cost;
					node_Row[popNode->x][popNode->y].direction = 3;
					pushHeap(&heap_node, &node_Row[popNode->x][popNode->y]);
				}
			}
			// 北西 (NW: 7)
			if (node_Row[popNode->x - 1][popNode->y + 1].cost != DIJK_WALLCOST && node_Row[popNode->x - 1][popNode->y + 1].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 7 ? CON_COST : DIAG_COST);
				if (node_Row[popNode->x - 1][popNode->y + 1].cost > next_cost)
				{
					node_Row[popNode->x - 1][popNode->y + 1].cost = next_cost;
					node_Row[popNode->x - 1][popNode->y + 1].direction = 7;
					pushHeap(&heap_node, &node_Row[popNode->x - 1][popNode->y + 1]);
				}
			}
			// 南西 (SW: 5)
			if (node_Row[popNode->x - 1][popNode->y].cost != DIJK_WALLCOST && node_Row[popNode->x - 1][popNode->y].isConfirm != 1)
			{
				int next_cost = popNode->cost + (popNode->direction == 5 ? CON_COST : DIAG_COST);
				if (node_Row[popNode->x - 1][popNode->y].cost > next_cost)
				{
					node_Row[popNode->x - 1][popNode->y].cost = next_cost;
					node_Row[popNode->x - 1][popNode->y].direction = 5;
					pushHeap(&heap_node, &node_Row[popNode->x - 1][popNode->y]);
				}
			}
		}
	}

	int toGool_direction = 0; 
    int N = 0;
    
    NODE_T *short_node;
    short_node = &node_Row[0][1]; // スタート位置
    toGool_direction = (short_node->direction + 4) % 8;

    for (int i = 0; i < 255; i++) {
        G_Short_Pass[i] = 0;
    }

    int final_x = 0;
    int final_y = 0;
    int final_dir = 0; 
    int last_dir = toGool_direction;


	while (1)
	{
		if(short_node->isRow == 1){
            short_node = &node_Row[short_node->x][short_node->y];
        }else{
            short_node = &node_Column[short_node->x][short_node->y];
        }

        if(short_node->cost == 0){
            final_x = short_node->x;
            final_y = short_node->y;
            final_dir = last_dir;
            break; // コスト0（ゴール）に到達したら抜ける
        }

		toGool_direction = (short_node->direction + 4) % 8;
		printf("%d direction\r\n", toGool_direction);
		if (toGool_direction == 0)
		{
			if (G_Short_Pass[N] < 0)
			{
				N++;
			}
			G_Short_Pass[N] += 2;
			short_node->y++;
		}
		else if (toGool_direction == 1)
		{
			N++;
			if (short_node->isRow == 1)
			{
				G_Short_Pass[N] = -3;
				short_node->x++;
				short_node->isRow = 0;
			}
			else
			{
				G_Short_Pass[N] = -2;
				short_node->y++;
				short_node->isRow = 1;
			}
		}
		else if (toGool_direction == 2)
		{
			if (G_Short_Pass[N] < 0)
			{
				N++;
			}
			G_Short_Pass[N] += 2;
			short_node->x++;
		}
		else if (toGool_direction == 3)
		{
			N++;
			if (short_node->isRow == 1)
			{
				G_Short_Pass[N] = -2;
				short_node->x++;
				short_node->y--;
				short_node->isRow = 0;
			}
			else
			{
				G_Short_Pass[N] = -3;
				short_node->isRow = 1;
			}
		}
		else if (toGool_direction == 4)
		{
			if (G_Short_Pass[N] < 0)
			{
				N++;
			}
			G_Short_Pass[N] += 2;
			short_node->y--;
		}
		else if (toGool_direction == 5)
		{
			N++;
			if (short_node->isRow == 1)
			{
				G_Short_Pass[N] = -3;
				short_node->y--;
				short_node->isRow = 0;
			}
			else
			{
				G_Short_Pass[N] = -2;
				short_node->x--;
				short_node->isRow = 1;
			}
		}
		else if (toGool_direction == 6)
		{
			if (G_Short_Pass[N] < 0)
			{
				N++;
			}
			G_Short_Pass[N] += 2;
			short_node->x--;
		}
		else if (toGool_direction == 7)
		{
			N++;
			if (short_node->isRow == 1)
			{
				G_Short_Pass[N] = -2;
				short_node->isRow = 0;
			}
			else
			{
				G_Short_Pass[N] = -3;
				short_node->x--;
				short_node->y++;
				short_node->isRow = 1;
			}
		}
	}

	if (G_Short_Pass[0] == 0)
	{
		G_Short_Pass[0] = -1;
	}

	Shortest_Pass_Compression();
	Shortest_Pass_Compression_NANAME();

	Maze_Mapping();

	const char *dir_str[] = {
		"North (0)", "North-East (1)", "East (2)", "South-East (3)",
		"South (4)", "South-West (5)", "West (6)", "North-West (7)"};

	printf("\n\r--- Final Robot State ---\n\r");
	printf("Position : X=%d, Y=%d\n\r", final_x, final_y);
	printf("Direction: %s\n\r", dir_str[final_dir]);

	for (int i = 0; i < 255; i++)
	{
		printf("PASS: %2d CP: %2d NANAME %3d \n\r", G_Short_Pass[i], G_Short_Pass_CP[i], G_Short_Pass_NANAME[i]);
	}

#ifdef SIM_EXPORT
	/* Machine-readable dump for the browser simulator (simulator/verify_against_c.mjs)
	   to diff its JS port against this actual compiled/executed program. Only
	   built when SIM_EXPORT is defined; the normal robot build is unaffected. */
	{
		FILE *fp = fopen("sim_export.json", "w");
		if (fp != NULL)
		{
			fprintf(fp, "{\n");

			fprintf(fp, "  \"mazeRow\": [");
			for (int j = 0; j < 17; j++)
				fprintf(fp, "%d%s", G_Maze_Row[j], j < 16 ? "," : "");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"mazeColumn\": [");
			for (int j = 0; j < 17; j++)
				fprintf(fp, "%d%s", G_Maze_Column[j], j < 16 ? "," : "");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"nodeRowCost\": [");
			for (int x = 0; x < 17; x++)
				for (int y = 0; y < 17; y++)
					fprintf(fp, "%d%s", node_Row[x][y].cost, (x == 16 && y == 16) ? "" : ",");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"nodeColumnCost\": [");
			for (int x = 0; x < 17; x++)
				for (int y = 0; y < 17; y++)
					fprintf(fp, "%d%s", node_Column[x][y].cost, (x == 16 && y == 16) ? "" : ",");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"shortPass\": [");
			for (int i = 0; i < 255; i++)
				fprintf(fp, "%d%s", G_Short_Pass[i], i < 254 ? "," : "");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"cp\": [");
			for (int i = 0; i < 255; i++)
				fprintf(fp, "%d%s", G_Short_Pass_CP[i], i < 254 ? "," : "");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"naname\": [");
			for (int i = 0; i < 255; i++)
				fprintf(fp, "%d%s", G_Short_Pass_NANAME[i], i < 254 ? "," : "");
			fprintf(fp, "],\n");

			fprintf(fp, "  \"finalX\": %d,\n", final_x);
			fprintf(fp, "  \"finalY\": %d,\n", final_y);
			fprintf(fp, "  \"finalDir\": %d\n", final_dir);
			fprintf(fp, "}\n");

			fclose(fp);
		}
	}
#endif

	return 0;
}

void Maze_setting()
{
	G_Maze_Row[0] = 0b1111111111111111;
	G_Maze_Row[1] = 0b1010110011110100;
	G_Maze_Row[2] = 0b0100110010101010;
	G_Maze_Row[3] = 0b1001110011010000;
	G_Maze_Row[4] = 0b0111100010101000;
	G_Maze_Row[5] = 0b0111110001010000;
	G_Maze_Row[6] = 0b0100001010101000;
	G_Maze_Row[7] = 0b0100000110010000;
	G_Maze_Row[8] = 0b0110001001100000;
	G_Maze_Row[9] = 0b0011010110110000;
	G_Maze_Row[10] = 0b0010111001110000;
	G_Maze_Row[11] = 0b0101010010100000;
	G_Maze_Row[12] = 0b0011000001011000;
	G_Maze_Row[13] = 0b0010110111010000;
	G_Maze_Row[14] = 0b0101110001110000;
	G_Maze_Row[15] = 0b0010101111111000;
	G_Maze_Row[16] = 0b1111111111111111;
	G_Maze_Column[0] = 0b1111111111111111;
	G_Maze_Column[1] = 0b0111111111111101;
	G_Maze_Column[2] = 0b1111111111011110;
	G_Maze_Column[3] = 0b0111101110101010;
	G_Maze_Column[4] = 0b0010110011010100;
	G_Maze_Column[5] = 0b0000000010101010;
	G_Maze_Column[6] = 0b0000000101010100;
	G_Maze_Column[7] = 0b0010001110101000;
	G_Maze_Column[8] = 0b0100101001011010;
	G_Maze_Column[9] = 0b0101110010111110;
	G_Maze_Column[10] = 0b0000100101011010;
	G_Maze_Column[11] = 0b1001001011000000;
	G_Maze_Column[12] = 0b0000010111000010;
	G_Maze_Column[13] = 0b0000101011000101;
	G_Maze_Column[14] = 0b0011011010101010;
	G_Maze_Column[15] = 0b0110101010100001;
	G_Maze_Column[16] = 0b1111111111111111;
}

void Maze_Mapping()
{
	for (int j = 16; j > 0; j--)
	{
		printf("   ");
		for (int i = 0; i < 16; i++)
		{
			printf("+");
			if ((G_Maze_Row[j] & (1 << i)) == (1 << i))
			{
				printf("---");
			}
			else
			{
				printf("%3d", node_Row[i][j].cost);
			}
		}
		printf("+\n\r");
		for (int i = 0; i < 17; i++)
		{
			if ((G_Maze_Column[i] & (1 << (j - 1))) == (1 << (j - 1)))
			{
				printf("   |");
			}
			else
			{
				printf(" %3d", node_Column[i][j - 1].cost); //
			}
		}
		printf("\n\r");
	}
	printf("   ");
	for (int i = 0; i < 16; i++)
	{
		printf("+");
		if ((G_Maze_Row[0] & (1 << i)) == (1 << i))
		{
			printf("---");
		}
		else
		{
			printf("   ");
		}
	}
	printf("＋\n\r");
}

void Shortest_Pass_Compression()
{
	for (i = 0; G_Short_Pass[i] != 0; i++)
	{
		G_Short_Pass_CP[i] = G_Short_Pass[i];
	}
	G_Short_Pass[i] = 1;
	G_Short_Pass_CP[i] = 1;
	G_Short_Pass_CP[0] = 1;
	for (i = 0; G_Short_Pass_CP[i] != 0; i++)
	{
		if (G_Short_Pass_CP[i] == -2)
		{
			if (G_Short_Pass_CP[i - 1] > 0)
			{
				if (G_Short_Pass[i + 1] > 0)
				{ // 左９０おおまわり
					G_Short_Pass_CP[i - 1] -= 1;
					G_Short_Pass_CP[i] = -4;
					G_Short_Pass_CP[i + 1] -= 1;
				}
				else if ((G_Short_Pass[i + 1] == -2) && (G_Short_Pass[i + 2] > 0))
				{ // 左１８０おおまわり
					G_Short_Pass_CP[i - 1] -= 1;
					G_Short_Pass_CP[i] = -5;
					G_Short_Pass_CP[i + 1] = -1;
					G_Short_Pass_CP[i + 2] -= 1;
				}
				if (G_Short_Pass_CP[i - 1] == 0)
				{
					G_Short_Pass_CP[i - 1] = -1;
				}
				if (G_Short_Pass_CP[i + 1] == 0)
				{
					G_Short_Pass_CP[i + 1] = -1;
				}
			}
		}
		else if (G_Short_Pass_CP[i] == -3)
		{
			if (G_Short_Pass_CP[i - 1] > 0)
			{
				if (G_Short_Pass_CP[i + 1] > 0)
				{ // 右９０おおまわり
					G_Short_Pass_CP[i - 1] -= 1;
					G_Short_Pass_CP[i] = -6;
					G_Short_Pass_CP[i + 1] -= 1;
				}
				else if ((G_Short_Pass[i + 1] == -3) && (G_Short_Pass[i + 2] > 0))
				{ // 右１８０おおまわり
					G_Short_Pass_CP[i - 1] -= 1;
					G_Short_Pass_CP[i] = -7;
					G_Short_Pass_CP[i + 1] = -1;
					G_Short_Pass_CP[i + 2] -= 1;
				}
				if (G_Short_Pass_CP[i - 1] == 0)
				{
					G_Short_Pass_CP[i - 1] = -1;
				}
				if (G_Short_Pass_CP[i + 1] == 0)
				{
					G_Short_Pass_CP[i + 1] = -1;
				}
			}
		}
	}
}

void Shortest_Pass_Compression_NANAME()
{
	for (i = 0; G_Short_Pass_CP[i] != 0; i++)
	{
		G_Short_Pass_NANAME[i] = G_Short_Pass_CP[i];
	}
	for (i = 0; G_Short_Pass_NANAME[i] != 0; i++)
	{
		if ((G_Short_Pass_NANAME[i] == -2) || (G_Short_Pass_NANAME[i] == -3))
		{
			if (G_Short_Pass_NANAME[i - 1] > 0)
			{ // 斜め入り
				if (G_Short_Pass_NANAME[i] == -2)
				{ // 左

					if (G_Short_Pass_NANAME[i + 1] == -3)
					{
						// 入り４５
						G_Short_Pass_NANAME[i - 1] -= 1;
						G_Short_Pass_NANAME[i] = -51;
						NANAME_Flag = 1;
						Pass_zero_act();
					}
					else if (G_Short_Pass_NANAME[i + 1] == -2)
					{
						// 入り135
						G_Short_Pass_NANAME[i - 1] -= 1;
						G_Short_Pass_NANAME[i] = -52;
						G_Short_Pass_NANAME[i + 1] = -1;
						// G_Short_Pass_NANAME[i + 2] = -1;
						NANAME_Flag = 1;
						Pass_zero_act();
					}
				}
				else if (G_Short_Pass_NANAME[i] == -3)
				{ // 右

					if (G_Short_Pass_NANAME[i + 1] == -2)
					{
						// 入り４５
						G_Short_Pass_NANAME[i - 1] -= 1;
						G_Short_Pass_NANAME[i] = -53;
						NANAME_Flag = 1;
						Pass_zero_act();
					}
					else if (G_Short_Pass_NANAME[i + 1] == -3)
					{
						// 入り135
						G_Short_Pass_NANAME[i - 1] -= 1;
						G_Short_Pass_NANAME[i] = -54;
						G_Short_Pass_NANAME[i + 1] = -1;
						// G_Short_Pass_NANAME[i + 2] = -1;
						NANAME_Flag = 1;
						Pass_zero_act();
					}
				}
			}
			else if ((G_Short_Pass_NANAME[i + 1] >= 0) && (NANAME_Flag == 1))
			{ // 斜め出 45
				if (G_Short_Pass_NANAME[i] == -3)
				{
					// 右４５
					// G_Short_Pass_NANAME[i - 1] = -1;
					G_Short_Pass_NANAME[i] = -63;
					G_Short_Pass_NANAME[i + 1] -= 1;
					Pass_zero_act();
				}
				else if (G_Short_Pass_NANAME[i] == -2)
				{
					// 左４５
					// G_Short_Pass_NANAME[i - 1] = -1;
					G_Short_Pass_NANAME[i] = -61;
					G_Short_Pass_NANAME[i + 1] -= 1;
					Pass_zero_act();
				}
				NANAME_Flag = 0;
			}
			else if ((G_Short_Pass_NANAME[i + 2] >= 0) && (NANAME_Flag == 1))
			{
				if (G_Short_Pass_NANAME[i] == G_Short_Pass_NANAME[i + 1])
				{
					if (G_Short_Pass_NANAME[i] == -3)
					{
						// 右135
						G_Short_Pass_NANAME[i] = -64;
						G_Short_Pass_NANAME[i + 1] = -1;
						G_Short_Pass_NANAME[i + 2] -= 1;
						Pass_zero_act();
					}
					else if (G_Short_Pass_NANAME[i] == -2)
					{
						// 左135
						G_Short_Pass_NANAME[i] = -62;
						G_Short_Pass_NANAME[i + 1] = -1;
						G_Short_Pass_NANAME[i + 2] -= 1;
						Pass_zero_act();
					}
					NANAME_Flag = 0;
				}
				else
				{
					if ((G_Short_Pass_NANAME[i] == -2) || (G_Short_Pass_NANAME[i] == -3))
					{
						if (G_Short_Pass_NANAME[i] == G_Short_Pass_NANAME[i + 1])
						{
							if (G_Short_Pass_NANAME[i] == -2)
							{
								G_Short_Pass_NANAME[i] = -65;
								G_Short_Pass_NANAME[i + 1] = -1;
							}
							else
							{
								G_Short_Pass_NANAME[i] = -66;
								G_Short_Pass_NANAME[i + 1] = -1;
							}
						}
						else
						{
							G_Short_Pass_NANAME[i] = -50;
							// G_Short_Pass_NANAME[i + 1] = -1;
						}
					}
				}
			}
			else if (NANAME_Flag == 1)
			{
				if ((G_Short_Pass_NANAME[i] == -2) || (G_Short_Pass_NANAME[i] == -3))
				{
					if (G_Short_Pass_NANAME[i] == G_Short_Pass_NANAME[i + 1])
					{
						if (G_Short_Pass_NANAME[i] == -2)
						{
							G_Short_Pass_NANAME[i] = -65;
							G_Short_Pass_NANAME[i + 1] = -1;
						}
						else
						{
							G_Short_Pass_NANAME[i] = -66;
							G_Short_Pass_NANAME[i + 1] = -1;
						}
					}
					else
					{
						G_Short_Pass_NANAME[i] = -50;
						// G_Short_Pass_NANAME[i + 1] = -1;
					}
				}
			}
		}
	}
	for (i = 0; G_Short_Pass_NANAME[i] != 0; i++)
	{
		if (G_Short_Pass_NANAME[i] == -50)
		{
			for (int j = 1; G_Short_Pass_NANAME[i + j] == -50; j++)
			{
				G_Short_Pass_NANAME[i + j] = -1;
				G_Short_Pass_NANAME[i] -= 50;
			}
		}
	}
}

void Pass_zero_act()
{
	if (G_Short_Pass_NANAME[i - 1] == 0)
	{
		G_Short_Pass_NANAME[i - 1] = -1;
	}
	if (G_Short_Pass_NANAME[i + 1] == 0)
	{
		G_Short_Pass_NANAME[i + 1] = -1;
	}
	if (G_Short_Pass_NANAME[i + 2] == 0)
	{
		G_Short_Pass_NANAME[i + 2] = -1;
	}

	// if (Known_Pass_CP[i - 1] == 0) {
	// 	Known_Pass_CP[i - 1] = -1;
	// }
	// if (Known_Pass_CP[i + 1] == 0) {
	// 	Known_Pass_CP[i + 1] = -1;
	// }
	// if (Known_Pass_CP[i + 2] == 0) {
	// 	Known_Pass_CP[i + 2] = -1;
	// }
}