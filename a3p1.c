// Assignment 3 Part 1
// Nathaniel Appiah, Pradhyuman Nandal

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int page;
    int dirty;
} Page;

typedef struct {
    int capacity;
    int size;
    int front;
    int rear;
    Page *arr; // having an array that stores data [pageNumber, dirtyBit] in the Queue
} Queue;


static Queue *create_queue(int capacity) 
{
    Queue *q = malloc(sizeof(*q));

    // checking if the memory allocation failed
    if (!q)  
    {
        return NULL;
    }

    q->arr = malloc(sizeof(Page) * capacity);

    if (!q->arr) 
    { 
        free(q); return NULL;
    }

    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = capacity - 1;
    return q;
}

static void free_queue(Queue *q) 
{
    if (!q) return;
    free(q->arr);
    free(q);
}

static int is_full(const Queue *q) 
{
    return q && (q->size == q->capacity);
}

static int is_empty(const Queue *q) 
{
    return !q || (q->size == 0);
}

static int enqueue(Queue *q, Page value) 
{
    if (!q || is_full(q)) return -1;
    q->rear = (q->rear + 1) % q->capacity;
    q->arr[q->rear] = value;
    q->size++;
    return 0;
}

static int dequeue(Queue *q, Page *out) 
{
    if (!q || is_empty(q)) return 0;
    Page val = q->arr[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    if (out) *out = val;
    return 1;
}

static int contains(const Queue *q, int pageNum) 
{
    if (!q || is_empty(q)) return 0;
    for (int i = 0; i < q->size; i++) {
        int idx = (q->front + i) % q->capacity;
        if (q->arr[idx].page == pageNum)
            return 1;
    }
    return 0;
}

static void set_page_dirty(Queue *q, int pageNum) 
{
    if (!q || is_empty(q))
    {
         return;
    }

    for (int i = 0; i < q->size; i++) {
        int idx = (q->front + i) % q->capacity;
        if (q->arr[idx].page == pageNum) {
            q->arr[idx].dirty = 1;
            break;
        }
    }
}

Page pages[16000];
int page_count = 0;

int main(void)
{
    // Reading the input
    char line[256];
    fgets(line, sizeof(line), stdin); // skip header

    while (fgets(line, sizeof(line), stdin)) {
        int pageNumber, dirtyBit;

        if (sscanf(line, "%d,%d", &pageNumber, &dirtyBit) == 2) {
            pages[page_count].page = pageNumber;
            pages[page_count].dirty = dirtyBit;
            page_count++;
        }
    }

    printf("+--------+--------------+--------------+\n");
    printf("| Frames | Page Faults  | Write-backs  |\n");
    printf("+--------+--------------+--------------+\n");

    // Run FIFO for 1–99 frames
    for (int f = 1; f < 100; f++) 
    {
        Queue *frames = create_queue(f);

        int page_faults = 0;
        int write_backs = 0;

        // going through each page
        for (int j = 0; j < page_count; j++) 
        {
            Page current = pages[j];

            if (!contains(frames, current.page)) 
            { // not in memory
                page_faults++;

                if (is_full(frames)) 
                {
                    Page evicted;
                    dequeue(frames, &evicted);
                    if (evicted.dirty == 1)
                        write_backs++;
                }

                enqueue(frames, current);
            } 
            else if (current.dirty == 1) 
            {
                set_page_dirty(frames, current.page);
            }
        }

        printf("| %6d | %12d | %12d |\n", f, page_faults, write_backs);
        free_queue(frames);
    }

    printf("+--------+--------------+--------------+\n");
    return 0;
}
