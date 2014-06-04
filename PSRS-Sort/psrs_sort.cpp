#include<stdio.h>
#include<stdlib.h>

#define SERIAL_VERSION 1

#ifndef SERIAL_VERSION
#define MAX_RADOME 100;
#define NUM 40;

typedef int Elem_type;

typedef struct{
	size_t left;
	size_t right;
} pair;

Elem_type* g_array;
size_t thread_num; // thread num
size_t n = NUM;
size_t gap;

void generate_list(Elem_type*, size_t);
void print_list(Elem_type*, size_t);
void elem_copy(Elem_type*, const Elem_type*, size_t);
//int fcmp(const void*, const void*);
void psrs_sort(Elem_type*, size_t);
void quick_sort(Elem_type*, size_t, size_t);

Elem_type* global_swap(size_t, Elem_type*, Elem_type*, size_t*);

int main(){
	
	printf("Please input the number of threads:");
	scanf("%ul", &thread_num);

	if(thread_num == 1)
		thread_num = 2;

	g_array = (Elem_type*) malloc(sizeof(Elem_type) * n);
	generate_list(g_array, n);
	printf("List before sorted:\n");
	print_list(g_array, n);
	//sort based on PSRS algorthm
	psrs_sort(g_array, n);
	//quick_sort(g_array, 0, n-1);
	printf("List after sorted:\n");
	print_list(g_array, n);
	system("pause");
	return 0;
	
}

void generate_list(Elem_type* pArray, size_t n) {
	size_t i;

	for(i = 0; i < n; i++)
		pArray[i] = rand() % MAX_RADOME;
}

void print_list(Elem_type* pArray, size_t n) {
	size_t i;

	for(i = 0; i < n; i++)
		printf("%d ", pArray[i]);
	printf("\n");
}


void psrs_sort(Elem_type* p_array, size_t n) {
	
	Elem_type* p_samples;
	Elem_type* p_meta_elems;
	Elem_type* snd_array;// temp array for store the interal result
	size_t i, j;
	
	snd_array = (Elem_type*) malloc(sizeof(Elem_type) * n);
	p_samples = (Elem_type*) malloc(sizeof(Elem_type) * thread_num * thread_num);
	p_meta_elems = (Elem_type*) malloc(sizeof(Elem_type) * (thread_num-1));
	gap =  n / thread_num;


	//quick sort for each segment
	for(i = 0; i < thread_num; i++)
		quick_sort(p_array, i * gap, (i + 1)*gap - 1);

	//sampling
	for(i = 0; i < thread_num; i++) {
		size_t sub_gap = gap / thread_num;
		for(j = 0; j < thread_num; j++)
			p_samples[i * thread_num + j] = p_array[i*gap + j*sub_gap];
	}

	for(i = 0; i < thread_num*thread_num; i++)
		printf("Sample:%d \n", p_samples[i]);
	quick_sort(p_samples, 0, thread_num * thread_num - 1);
	//sampling
	for(i = 0; i < thread_num-1; i++)
		p_meta_elems[i] = p_samples[(i+1) * thread_num];

	for(i = 0; i < thread_num-1; i++)
		printf("Meta Elems:%d \n", p_meta_elems[i]);

	//global swap and merge sort
	elem_copy(snd_array, p_array, n);
	print_list(snd_array, n);
	size_t cursor = 0;
	for(i = 0; i < thread_num; i++){		
		size_t size;
		Elem_type* p_segment = global_swap(i, snd_array, p_meta_elems, &size);		
		elem_copy(p_array+cursor, p_segment, size);
		cursor += size;
		free(p_segment);
		p_segment = NULL;
	}
	
	free(snd_array);
	free(p_samples);
	free(p_meta_elems);
}

Elem_type* global_swap(size_t rank, Elem_type* p_array, Elem_type* p_meta_elems, size_t* size) {

	Elem_type* segment;
	pair* p_pairs = (pair*) malloc(sizeof(pair) * thread_num);
	size_t i, j, s_size;
	
	if(rank == 0) {

		printf("Target: %d\n", p_meta_elems[rank]);
		for(i = 0; i < thread_num; i++){
			p_pairs[i].left = i*gap;
			for(j = i*gap; (j < (i+1) * gap) && p_array[j] <= p_meta_elems[rank];j++)
				;
			p_pairs[i].right = j - 1;
		}
			
	}else if(rank < thread_num-1) {
		for(i = 0; i < thread_num; i++){
			for(j = i*gap; (j < (i+1) * gap) && p_array[j] <= p_meta_elems[rank-1];j++)
				;
			p_pairs[i].left = j;
			for(;(j < (i+1) * gap) && p_array[j] <= p_meta_elems[rank];j++)
				;
			p_pairs[i].right = j - 1;
		}
	}else {
		for(i = 0; i < thread_num; i++){
			p_pairs[i].right = (i + 1)*gap -1;
			for(j = (i+1)*gap - 1; (j >= i * gap) && p_array[j] > p_meta_elems[rank-1];j--)
				;
			p_pairs[i].left = j+1;
		}
	}

	for(i = 0, s_size = 0; i < thread_num; i++) {
		s_size += p_pairs[i].right - p_pairs[i].left + 1;
		printf("left %d | right %d\n", p_pairs[i].left, p_pairs[i].right);
	}

	*size = s_size;

	printf("Size of segment:%d\n", s_size);

	segment = (Elem_type*) malloc(sizeof(Elem_type) * s_size);

	//merge sort
	for(i = 0; i < s_size; i++) {
		Elem_type min;
		size_t min_index;
		//find the first element as the min element
		for(j = 0; j < thread_num; j++)
			if(p_pairs[j].left <= p_pairs[j].right) {
				min_index = j;
				min = p_array[p_pairs[j].left];
				break;
			}
		//find the miniest element	
		for(j = min_index+1; j < thread_num; j++)
			if(p_pairs[j].left <= p_pairs[j].right 
					&& p_array[p_pairs[j].left] < min) {
				min_index = j;
				min = p_array[p_pairs[j].left];
			}
		segment[i] = min;
		p_pairs[min_index].left++;
		printf("Min in this round: %d\n", segment[i]);
	}

	free(p_pairs);

	return segment;
}

void elem_copy(Elem_type* to, const Elem_type* from, size_t n){
	size_t i;
	for(i = 0; i < n; i++)
		to[i] = from[i];
}

//void quick_sort(Elem_type* p_array, size_t left, size_t right) {
//
//	if(left >= right)
//		return;
//
//	size_t l = left + 1;
//	size_t r = right;
//
//	Elem_type target = p_array[left];
//	while(1) {
//		while(l <= right && p_array[l] <= target) l++;
//		while(r > (left + 1) && p_array[r] > target) r--;
//
//		if(l >= r)
//			break;
//		Elem_type temp = p_array[l];
//		p_array[l] = p_array[r];
//		p_array[r] = temp;
//	}
//
//	Elem_type temp = p_array[left];
//	p_array[left] = p_array[r];
//	p_array[r] = temp;
//
//	quick_sort(p_array, left, r-1);
//	quick_sort(p_array, r+1, right);
//}

void quick_sort (Elem_type* data, size_t left, size_t right) {
	size_t m = left + (right - left) / 2;
	Elem_type pivot = data[m];
	size_t i = left,j = right;
	for ( ; i < j;) {
		while (! (i>= m || pivot < data[i]))
			++i;
		if (i < m) {
			data[m] = data[i];
			m = i;
		}
		while (! (j <= m || data[j] < pivot))
			--j;
		if (j > m) {
			data[m] = data[j];
			m = j;
		}
	}
	data[m] = pivot;
	if (m - left > 1)
		quick_sort(data, left, m - 1);
	if (right - m > 1)
		quick_sort(data, m + 1, right);
}
 
#endif