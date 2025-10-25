// Assignment 3 - Page Replacement Algorithms
// Nathaniel Appiah, Pradhyuman Nandal

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Structure to represent a page with its page number and dirty bit
typedef struct 
{
    int page;
    int dirty;
} Page;

// Queue structure for FIFO algorithm
typedef struct {
    int capacity;
    int size;
    int front;
    int rear;
    Page *arr;  // Array that stores data [pageNumber, dirtyBit] in the Queue
} Queue;

// Frame list structure for OPT algorithm
typedef struct
{
    int capacity;
    int size;
    Page *frame;
    int *order;  // FIFO tie-breaking order
} FrameList;

// Clock page structure with reference bits for Second Chance algorithm
typedef struct {
    int page;
    int dirty;
    unsigned int ref_bits;  // n-bit reference register
} ClockPage;

// Clock frame list structure for Second Chance algorithm
typedef struct {
    int capacity;
    int size;
    int hand;  // Clock hand position
    ClockPage *frame;
} ClockFrameList;

Page pages[16000];
int page_count = 0;

//  FIFO Algorithm Functions 

// Create a queue with given capacity
static Queue *create_queue(int capacity)
{
    Queue *q = malloc(sizeof(*q));  // Allocate memory for the queue structure
    if (!q) return NULL;

    q->arr = malloc(sizeof(Page) * capacity);  // Allocate memory for the array
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

// Free queue memory
static void free_queue(Queue *q) 
{
    if (!q) return;
    free(q->arr);
    free(q);
}

// Check if queue is full
static int is_full(const Queue *q) 
{
    return (q != NULL && q->size == q->capacity);
}

// Check if queue is empty
static int is_empty(const Queue *q) 
{
    return (q == NULL || q->size == 0);
}

// Add element to queue
static int enqueue(Queue *q, Page value) 
{
    if (q == NULL || is_full(q))
        return -1;

    q->rear = (q->rear + 1) % q->capacity;
    q->arr[q->rear] = value;
    q->size++;
    return 0;
}

// Remove element from queue
static int dequeue(Queue *q, Page *out) 
{
    if (q == NULL || is_empty(q))
        return -1;
        
    Page val = q->arr[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    if (out) *out = val;
    return 1;
}

// Check if page exists in queue
static int contains(const Queue *q, int pageNum) 
{
    if (q == NULL || is_empty(q))
        return 0;
        
    for (int i = 0; i < q->size; i++) {
        int idx = (q->front + i) % q->capacity;
        if (q->arr[idx].page == pageNum)
            return 1;
    }
    return 0;
}

// Mark a page as dirty in the queue
static void set_page_dirty(Queue *q, int pageNum)
{
    if (!q || is_empty(q))
        return;

    for (int i = 0; i < q->size; i++) {
        int idx = (q->front + i) % q->capacity;
        if (q->arr[idx].page == pageNum) {
            q->arr[idx].dirty = 1;
            break;
        }
    }
}

//  OPT Algorithm Functions 

// Create a frame list with given capacity
static FrameList *create_frameList(int capacity)
{
    FrameList *fl = malloc(sizeof(*fl));
    if (!fl) return NULL;

    fl->frame = malloc(sizeof(Page) * capacity);
    fl->order = malloc(sizeof(int) * capacity);
    
    if (!fl->frame || !fl->order)
    {
        free(fl->frame);
        free(fl->order);
        free(fl);
        return NULL;
    }

    fl->capacity = capacity;
    fl->size = 0;

    return fl;
}

// Free frame list memory
static void free_frameList(FrameList *fl)
{
    if (!fl) return;
    free(fl->frame);
    free(fl->order);
    free(fl);
}

// Helper function for OPT: find next use of a page in the future
// Returns INT_MAX if page is never used again
static int find_next_use(int curr_index, int page)
{
    for (int i = curr_index + 1; i < page_count; i++)
    {
        if (pages[i].page == page)
            return i;
    }
    return INT_MAX;  // Page not used again
}

// Clock Algorithm Functions

// Create a clock frame list with given capacity

static ClockFrameList *create_clock_frameList(int capacity)
{
    ClockFrameList *cfl = malloc(sizeof(*cfl));
    if (!cfl) return NULL;

    cfl->frame = malloc(sizeof(ClockPage) * capacity);
    if (!cfl->frame)
    {
        free(cfl);
        return NULL;
    }

    cfl->capacity = capacity;
    cfl->size = 0;
    cfl->hand = 0;
    
    // Initialize all frames
    for (int i = 0; i < capacity; i++)
    {
        cfl->frame[i].page = -1;
        cfl->frame[i].dirty = 0;
        cfl->frame[i].ref_bits = 0;
    }
    
    return cfl;
}

// Free clock frame list memory
static void free_clock_frameList(ClockFrameList *cfl)
{
    if (!cfl) return;
    free(cfl->frame);
    free(cfl);
}

// Check if page exists in clock frames and return its index
static int contains_clock_frame(ClockFrameList *cfl, int pageNum, int *index)
{
    for (int i = 0; i < cfl->size; i++)
    {
        if (cfl->frame[i].page == pageNum)
        {
            if (index) *index = i;
            return 1;
        }
    }
    return 0;
}

// Shift all reference registers right by 1 bit
static void shift_reference_bits(ClockFrameList *cfl, int n)
{
    for (int i = 0; i < cfl->size; i++)
    {
        cfl->frame[i].ref_bits >>= 1;
    }
}

// Set the leftmost  bit of the reference register to 1
static void set_reference_bit(ClockFrameList *cfl, int index, int n)
{
    unsigned int mask = 1U << (n - 1); 
    cfl->frame[index].ref_bits |= mask;  // Using bitmask with OR to set the bit
}

// Find victim page using Second Chance algorithm
// Returns the index of the page to be replaced
static int find_victim_clock(ClockFrameList *cfl, int n)
{
    // Create mask for n bits (all 1s for n bits)
    unsigned int n_bit_mask = (1U << n) - 1;
    int start_hand = cfl->hand;  // Hand points towards a frame currently being examined
    
    while (1)
    {
        // Check if all reference bits are 0
        if ((cfl->frame[cfl->hand].ref_bits & n_bit_mask) == 0)
        {
            // Found victim with all 0s
            int victim = cfl->hand;
            cfl->hand = (cfl->hand + 1) % cfl->size;
            return victim;
        }
        
        // Give second chance: shift right by 1
        cfl->frame[cfl->hand].ref_bits >>= 1;
        cfl->hand = (cfl->hand + 1) % cfl->size;
        
        // Prevent infinite loop (safety check)
        if (cfl->hand == start_hand)
        {
            // If we've gone full circle, just pick current position
            int victim = cfl->hand;
            cfl->hand = (cfl->hand + 1) % cfl->size;
            return victim;
        }
    }
}

int main(int argc, char *argv[])
{
    // Check if the user provided the correct number of arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s FIFO|OPT|CLK < inputfile.csv\n", argv[0]);
        return 1;
    }

    // Read input from stdin
    char line[256];
    fgets(line, sizeof(line), stdin);  // Skip header

    // Read all pages into global array
    while (fgets(line, sizeof(line), stdin)) {
        int pageNumber, dirtyBit;
        if (sscanf(line, "%d,%d", &pageNumber, &dirtyBit) == 2) {
            pages[page_count].page = pageNumber;
            pages[page_count].dirty = dirtyBit;
            page_count++;
        }
    }

    //  FIFO ALGORITHM 
    if (strcmp(argv[1], "FIFO") == 0)
    {
        printf("FIFO\n");
        printf("+--------+--------------+--------------+\n");
        printf("| Frames | Page Faults  | Write-backs  |\n");
        printf("+--------+--------------+--------------+\n");

        // Run simulation for 1 to 100 frames
        for (int f = 1; f <= 100; f++) 
        {
            Queue *frames = create_queue(f);
            int page_faults = 0;
            int write_backs = 0;

            // Process each page reference
            for (int j = 0; j < page_count; j++) 
            {
                Page current = pages[j];

                // Check if page is not in memory
                if (!contains(frames, current.page)) 
                { 
                    page_faults++;

                    // If frames are full, evict the oldest page (FIFO)
                    if (is_full(frames)) 
                    {
                        Page evicted;
                        dequeue(frames, &evicted);
                        if (evicted.dirty == 1)
                            write_backs++;
                    }

                    // Add new page to frames
                    enqueue(frames, current);
                } 
                else if (current.dirty == 1) 
                {
                    // Page hit - mark as dirty if current reference is dirty
                    set_page_dirty(frames, current.page);
                }
            }
            printf("| %6d | %12d | %12d |\n", f, page_faults, write_backs);
            free_queue(frames);
        }
        printf("+--------+--------------+--------------+\n");
    }

    // OPTIMAL ALGORITHM
    else if (strcmp(argv[1], "OPT") == 0)
    {
        printf("OPT\n");
        printf("+--------+--------------+--------------+\n");
        printf("| Frames | Page Faults  | Write-backs  |\n");
        printf("+--------+--------------+--------------+\n");

        // Run simulation for 1 to 100 frames
        for (int f = 1; f <= 100; f++)
        {
            // Use simple arrays for frame management
            int frame_pages[100];
            int frame_dirty[100];
            int frame_order[100];  // For FIFO tie-breaking
            int used = 0;
            int timestamp = 0;  // Tracks insertion order for tie-breaking
            
            // Initialize frames to -1
            for (int x = 0; x < f; x++) {
                frame_pages[x] = -1;
                frame_dirty[x] = 0;
                frame_order[x] = 0;
            }
            
            int page_faults = 0;
            int write_backs = 0;

            // Process each page reference
            for (int i = 0; i < page_count; i++)
            {
                int pg = pages[i].page;
                int d = pages[i].dirty;
                int hit = -1;

                // Check if page is in frames
                for (int x = 0; x < used; x++)
                {
                    if (frame_pages[x] == pg)
                    {
                        hit = x;
                        break;
                    }
                }

                if (hit != -1)
                {
                    // Page hit - update dirty bit if needed
                    if (d == 1)
                    {
                        frame_dirty[hit] = 1;
                    }
                }
                else
                {
                    // Page fault
                    page_faults++;

                    if (used < f)
                    {
                        // Frames not full - just add
                        frame_pages[used] = pg;
                        frame_dirty[used] = d;
                        frame_order[used] = timestamp++;
                        used++;
                    }
                    else
                    {
                        // Frames full - find victim using optimal algorithm
                        int victim = 0;
                        int farthest = -1;
                        int oldest_order = INT_MAX;

                        // Find page with farthest next use
                        for (int x = 0; x < f; x++)
                        {
                            int next = find_next_use(i, frame_pages[x]);

                            // Choose page with farthest next use
                            if (next > farthest)
                            {
                                farthest = next;
                                victim = x;
                                oldest_order = frame_order[x];
                            }
                            // Tie-breaking: use FIFO order (oldest first)
                            else if (next == farthest)
                            {
                                if (frame_order[x] < oldest_order)
                                {
                                    victim = x;
                                    oldest_order = frame_order[x];
                                }
                            }
                        }

                        // Evict victim and write back if dirty
                        if (frame_dirty[victim] == 1)
                        {
                            write_backs++;
                        }

                        // Replace victim with new page
                        frame_pages[victim] = pg;
                        frame_dirty[victim] = d;
                        frame_order[victim] = timestamp++;
                    }
                }
            }

            printf("| %6d | %12d | %12d |\n", f, page_faults, write_backs);
        }
        printf("+--------+--------------+--------------+\n");
    }

    // SECOND CHANCE (CLOCK) ALGORITHM 
    else if (strcmp(argv[1], "CLK") == 0)
    {
        int frames = 50;  // Fixed at 50 frames for Second Chance

        //  Experiment 1: m=10, vary n from 1 to 32
        printf("CLK, m=10\n");
        printf("+--------+--------------+--------------+\n");
        printf("| n      | Page Faults  | Write-backs  |\n");
        printf("+--------+--------------+--------------+\n");

        int m = 10;  // Shift reference bits every 10 references
        for (int n = 1; n <= 32; n++)  // n = number of bits in reference register
        {
            ClockFrameList *cfl = create_clock_frameList(frames);
            int page_faults = 0;
            int write_backs = 0;
            int ref_counter = 0;  // Counter for shifting reference bits

            // Process each page reference
            for (int i = 0; i < page_count; i++)
            {
                int current_page = pages[i].page;
                int current_dirty = pages[i].dirty;
                int page_index = -1;

                // Check if page is already in memory
                if (contains_clock_frame(cfl, current_page, &page_index))
                {
                    // Page hit - set reference bit and update dirty flag
                    set_reference_bit(cfl, page_index, n);
                    if (current_dirty == 1)
                        cfl->frame[page_index].dirty = 1;
                }
                else
                {
                    // Page fault
                    page_faults++;

                    if (cfl->size < cfl->capacity)
                    {
                        // Frames not full - just add the page
                        int idx = cfl->size;
                        cfl->frame[idx].page = current_page;
                        cfl->frame[idx].dirty = current_dirty;
                        cfl->frame[idx].ref_bits = 0;
                        set_reference_bit(cfl, idx, n);
                        cfl->size++;
                    }
                    else
                    {
                        // Frames full - find a victim page
                        int victim_idx = find_victim_clock(cfl, n);
                        
                        // Write back if victim page is dirty
                        if (cfl->frame[victim_idx].dirty == 1)
                            write_backs++;

                        // Replace victim with new page
                        cfl->frame[victim_idx].page = current_page;
                        cfl->frame[victim_idx].dirty = current_dirty;
                        cfl->frame[victim_idx].ref_bits = 0;
                        set_reference_bit(cfl, victim_idx, n);
                    }
                }

                // Shift reference bits after m references
                ref_counter++;
                if (ref_counter >= m)
                {
                    shift_reference_bits(cfl, n);
                    ref_counter = 0;
                }
            }

            printf("| %6d | %12d | %12d |\n", n, page_faults, write_backs);
            free_clock_frameList(cfl);
        }
        printf("+--------+--------------+--------------+\n\n");

        // Experiment 2: n=8, vary m from 1 to 100 
        printf("CLK, n=8\n");
        printf("+--------+--------------+--------------+\n");
        printf("| m      | Page Faults  | Write-backs  |\n");
        printf("+--------+--------------+--------------+\n");

        int n = 8;  // Fixed at 8 bits
        for (int m = 1; m <= 100; m++)  // m = shift interval
        {
            ClockFrameList *cfl = create_clock_frameList(frames);
            int page_faults = 0;
            int write_backs = 0;
            int ref_counter = 0;

            // Process each page reference
            for (int i = 0; i < page_count; i++)
            {
                int current_page = pages[i].page;
                int current_dirty = pages[i].dirty;
                int page_index = -1;

                // Check if page is already in memory
                if (contains_clock_frame(cfl, current_page, &page_index))
                {
                    // Page hit - set reference bit and update dirty flag
                    set_reference_bit(cfl, page_index, n);
                    if (current_dirty == 1)
                        cfl->frame[page_index].dirty = 1;
                }
                else
                {
                    // Page fault
                    page_faults++;

                    if (cfl->size < cfl->capacity)
                    {
                        // Frames not full - just add the page
                        int idx = cfl->size;
                        cfl->frame[idx].page = current_page;
                        cfl->frame[idx].dirty = current_dirty;
                        cfl->frame[idx].ref_bits = 0;
                        set_reference_bit(cfl, idx, n);
                        cfl->size++;
                    }
                    else
                    {
                        // Frames full - find a victim page
                        int victim_idx = find_victim_clock(cfl, n);
                        
                        // Write back if victim page is dirty
                        if (cfl->frame[victim_idx].dirty == 1)
                            write_backs++;

                        // Replace victim with new page
                        cfl->frame[victim_idx].page = current_page;
                        cfl->frame[victim_idx].dirty = current_dirty;
                        cfl->frame[victim_idx].ref_bits = 0;
                        set_reference_bit(cfl, victim_idx, n);
                    }
                }

                // Shift reference bits after m references
                ref_counter++;
                if (ref_counter >= m)
                {
                    shift_reference_bits(cfl, n);
                    ref_counter = 0;
                }
            }

            printf("| %6d | %12d | %12d |\n", m, page_faults, write_backs);
            free_clock_frameList(cfl);
        }
        printf("+--------+--------------+--------------+\n");
    }

    // Invalid algorithm specified
    else
    {
        fprintf(stderr, "Unknown algorithm: %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}