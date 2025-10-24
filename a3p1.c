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
    Queue *q = malloc(sizeof(*q)); // allocating memory of the same size of the struct Queue

    // checking if the memory allocation failed
    if (!q)  
    {
        return NULL;
    }

    q->arr = malloc(sizeof(Page) * capacity); // allocating memory for the array that depends upon the for loop down below

    // checking if the memory allocation for the array failed
    if (!q->arr) 
    { 
        free(q); 
        return NULL;
    }

    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = capacity - 1;
    return q;
}

static void free_queue(Queue *q) 
{
    if (!q)
    {
        return;
    } 
    free(q->arr);
    free(q);
}

static int is_full(const Queue *q) 
{
    if (q != NULL && q->size == q->capacity)
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

static int is_empty(const Queue *q) 
{
    if (q == NULL || q->size == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int enqueue(Queue *q, Page value) 
{
    if (q == NULL || is_full(q) == 1)
    {
        return -1;
    }

    q->rear = (q->rear + 1) % q->capacity;
    q->arr[q->rear] = value;
    q->size++;
    return 0;
}

static int dequeue(Queue *q, Page *out) 
{
    if (q == NULL || is_empty(q) == 1)
    {
        return -1;
    }
    Page val = q->arr[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    if (out) 
    {
        *out = val;
    }
    return 1;
}

static int contains(const Queue *q, int pageNum) 
{
    if (q == NULL || is_empty(q) == 1)
    {
        return 0;
    } 
    for (int i = 0; i < q->size; i++) {
        int idx = (q->front + i) % q->capacity;
        if (q->arr[idx].page == pageNum)
            return 1;
    }
    return 0;
}

static void set_page_dirty(Queue *q, int pageNum) // currentPage will be used for the pageNum down below.
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

    printf("FIFO\n");
    printf("+--------+--------------+--------------+\n");
    printf("| Frames | Page Faults  | Write-backs  |\n");
    printf("+--------+--------------+--------------+\n");

    // FIFO Algorithm
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
            { 
                // not in memory
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
        printf("| %6d | %12d | %12d |\n", f+1, page_faults, write_backs);
        free_queue(frames);
    }
    printf("+--------+--------------+--------------+\n");

    printf("\n");

    printf("OPT\n");
    printf("+--------+--------------+--------------+\n");
    printf("| Frames | Page Faults  | Write-backs  |\n");
    printf("+--------+--------------+--------------+\n");

    // Optimal Algorithm
    for (int f = 1; f <= 100; f++)
    {
        Queue *frames = create_queue(f);

        int page_faults = 0;
        int write_backs = 0;

        for (int i = 0; i < page_count; i++)
        {
            Page current = pages[i];

            if (!contains(frames, current.page)) 
            { 
                // not in memory and therefore a pagefault
                page_faults++;

                if (is_full(frames)) // check if the frame is full, if yes we need to find the victim page
                {
                    int farthest_index = -1;
                    int victim_index = -1;

                    // iterating through frames
                    for (int x = 0; x < frames->size; x++)  
                    {
                        int index = (frames->front + x) % frames->capacity; // this makes sure that we iterate through each element properly
                        int page_in_frame = frames->arr[index].page;

                        int next_occurence = -1;

                        // iterating through pages for each existing page in the frame
                        for (int k = i + 1; k < page_count; k++)
                        {
                            if (pages[k].page == page_in_frame)
                            {
                                next_occurence = k;
                                break;
                            }
                        }

                        if (next_occurence == -1)
                        {
                            victim_index = index;
                            break;
                        }

                        if (next_occurence > farthest_index)
                        {
                            farthest_index = next_occurence;
                            victim_index = index;
                        }
                    }

                    Page evicted = frames->arr[victim_index];

                    if (evicted.dirty == 1)
                    {
                        write_backs++;
                    }

                    frames->arr[victim_index] = current;
                }
                else
                {
                    enqueue(frames, current);
                }
            } 
            else if (current.dirty == 1) 
            {
                set_page_dirty(frames, current.page);
            }
        }
        printf("| %6d | %12d | %12d |\n", f+1, page_faults, write_backs);
        free_queue(frames);
    }
    printf("+--------+--------------+--------------+\n");
    return 0;
}
