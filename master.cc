
#include "mpitracer.h"

extern int errno; 

using namespace std; 

namespace danzer {

// This module reads layouts of a single file, generates and pushes tasks into corresponding Task_queue. If Task_queue is full, it sends tasks to Reader Process through MPI_SEND command. 
// void Dedupe::layout_analysis(filesystem::directory_entry entry, vector<vector<object_task*>> &task_queue){
// //void layout_analysis(fs::directory_entry entry){
// //int main(){

// 	chrono::high_resolution_clock::time_point start_time, end_time; 
// 	static int traverse_cnt = 0; 
// 	if (traverse_cnt == 0)
// 	{
// 		traverse_cnt ++; 
// 		cout << "Hello world!\n"; 
// 		start_time = chrono::high_resolution_clock::now();
// 	}
// 	int rc[5];
// 	uint64_t count = 0, size, start, end, idx, offset, interval, file_size;  
// 	struct stat64 sb; 
// 	object_task * task;
// 	int worldSize = this->worldSize; 
// 	int readerNum = 10; // TODO should be checked during runtime

// 	//vector <vector <struct object_task*>> task_queue(OST_NUMBER); 
	
// 	static int file_idx = 0;
// 	int obj_idx = 0; 

// 	// Get layouts of the file 
// 	struct llapi_layout * layout; 
// 	layout = llapi_layout_get_by_path(entry.path().c_str(), 0); 
// 	if (layout == NULL){
// 		cout << "errno:" << errno; 
// 		return; 
// 		// continue; 
// 	}
	
// 	// Get size of the file. 
// 	stat64(entry.path().c_str(), &sb);
// 	file_size = sb.st_size; 

// 	total_file_size += file_size; 


// 	// Get first layout of the file 
// 	rc[0] = llapi_layout_comp_use(layout, 1);
// 	if (rc[0]){
// 		cout << "error: layout component iteration failed\n";
// 		return; 
// 		//continue; 
// 	}
// 	while(1){

// 		// Get stripe count, stripe size, and range(in terms of the size) of the layout. 
// 		rc[0] = llapi_layout_stripe_count_get(layout, &count);
// 		rc[1] = llapi_layout_stripe_size_get(layout, &size); 
// 		rc[2] = llapi_layout_comp_extent_get(layout, &start, &end);
// 		if (rc[0] || rc[1] || rc[2]) 
// 		{
// 			cout << "error: cannot get stripe information\n"; 
// 			continue; 
// 		}

		
// 		interval = count * size; 
// 		end = (end < file_size)? end : file_size; 
// 		// For each stripe, Get OST index and create Task that contains the information about what to read that will be sent to each Reader Process 
// 		for (int i = 0; i < count; i ++)
// 		{	
// 			// Get OST index 
// 			rc[0] = llapi_layout_ost_index_get(layout, i, &idx); 
// 			// If OST information cannot be taken, it means that we get all the OST idx that stores the object of the file and we can break out of the loop. 
// 			if (rc[0]){
// 				//cout << "error: cannot get ost index\n"; 
// 				goto here_exit; 
// 			}
			
// 			// Generate the task and push it into corresponding task_queue (We allocate queue as much as the number of the OST). 
// 			task = object_task_generate(entry.path().c_str(), idx, start + i * size, end, interval, size);
// 			this->task_cnt ++; 

// 			task_queue[task->ost].push_back(task); 


// 			// load balancing code 
// 			uint64_t object_task_size = (end - start) / count; 
// 			this->size_per_ost[task->ost] += object_task_size; 
// 			this->size_per_rank[task->ost % (worldSize-1) + 1] += object_task_size; 



// 			// If task queue is full, then we send the tasks in the queue to corresponding Reader Process. 
// 			if (!load_balance && task_queue[task->ost].size() == TASK_QUEUE_FULL)
// 			{
		
// 				char * Msg = object_task_queue_clear(task_queue[task->ost], NULL); 
				
// 				// Send Msg to Read Processes(whose rank is OST_NUMBER & Read_Process_Num)
// 				// MPI_SEND
// 				int rc = MPI_Ssend(Msg, sizeof(object_task) * TASK_QUEUE_FULL, MPI_CHAR, task->ost % (worldSize-1) + 1, TASK_QUEUE_FULL, MPI_COMM_WORLD); 
// 				if (rc != MPI_SUCCESS)
// 					cout << "MPI Send Failed\n"; 
			
// 				//free(Msg); 
// 			//	delete(Msg); 
// 				delete[] Msg;

// 			}	
// 		}
// 		rc[0] = llapi_layout_comp_use (layout, 3); 
// 		if (rc[0] == 0)
// 			continue; 
// 		if (rc[0] < 0)
// 			cout << "error: layout component iteration failed\n"; 
// 		break; 
// 	}
// 	here_exit:
// 		return; 
// 		//continue;
// }

void Dedupe::file_task_end_of_process(vector<File_task_queue> & file_task_queue)
{
	char termination_task[20];
	strcpy(termination_task, TERMINATION_MSG);

	int turn = 0; 
	
	if (!load_balance){
		for (auto q : file_task_queue)
		{
			if (q.cnt != 0)
			{
				int rc = MPI_Send(q.queue.c_str(), q.queue.length()+1, MPI_CHAR, turn % (worldSize-1) + 1, q.cnt, MPI_COMM_WORLD); 
				if (rc != MPI_SUCCESS)
					cout << "MPI Send Failed\n";
			}
		
		
			printf("total_file_size\t%lld\n", q.total_file_size); 
		
		}


		


	}
	for(int i=1; i < worldSize; i++) {
    	MPI_Send(termination_task, sizeof(TERMINATION_MSG), MPI_CHAR, i, 0, MPI_COMM_WORLD);
		cout << "termination msg sent\n";
	}

}

void Dedupe::layout_end_of_process(vector<vector<object_task*>> &task_queue){
	int task_num;
	int worldSize = this->worldSize;
	char termination_task[20];
	strcpy(termination_task, TERMINATION_MSG);


	for (int ost = 0; ost < OST_NUMBER; ost++)
	{
		char * Msg = object_task_queue_clear(task_queue[ost], &task_num); 
		if(Msg == nullptr){
			cout << "Msg is NULL\n";
			continue;
		}	
		int rc = MPI_Send(Msg, sizeof(object_task) * task_num, MPI_CHAR, ost % (worldSize-1) + 1, task_num, MPI_COMM_WORLD); 
		//int rc = MPI_Ssend(Msg, sizeof(object_task) * TASK_QUEUE_FULL, MPI_CHAR, ost % (worldSize-1) + 1, task_num, MPI_COMM_WORLD); 
		if (rc != MPI_SUCCESS)
			cout << "MPI Send Failed\n";
		
		object_task_buffer_free(Msg); 

	}
	for(int i=1; i < worldSize; i++) {
    	MPI_Send(termination_task, sizeof(TERMINATION_MSG), MPI_CHAR, i, 0, MPI_COMM_WORLD);
		cout << "termination msg sent\n";
	}
}

// Push IDX th BUFFER into Large MSG data structure. BUFFER is at the IDX th position on the Msg 
// BUFFER is the serialized string of the struct object_task. 
void Dedupe::Msg_Push(char * buffer, char * Msg, int idx){
	memcpy(Msg + idx * sizeof(object_task), buffer, sizeof(object_task)); 
}



void Dedupe::file_task_distribution(std::filesystem::directory_entry entry, vector<File_task_queue> & file_task_queue, vector<pair<string, uint64_t>> &file_task_list)
{
	static int turn = 0 ; 


	struct stat64 sb; 
	int workerNumber = worldSize-1 ; 

	// Test code for Random Scheduling & Synthetic Dataset 
	static int task_remaining_workers=workerNumber; 
	int NUMFILES = 576; 
	int MAX_FILE_NUM_PER_WORKER = 576 / workerNumber; 

	
	srand((unsigned int)time(NULL)); 
	
	string file_path = entry.path(); 
	
	stat64(entry.path().c_str(), &sb);
	uint64_t file_size = sb.st_size; 

	
	// Append current file to the task queue for rank TURN.
	file_task_queue[turn].queue += file_path;
	file_task_queue[turn].queue += '\n'; 
	file_task_queue[turn].cnt += 1; 
	file_task_queue[turn].total_file_size += file_size; 		

	
	// Test code for Random Scheduling
	if (file_task_queue[turn].cnt == MAX_FILE_NUM_PER_WORKER) 
	{
		task_remaining_workers -= 1;
	}



	file_task_list.push_back(make_pair(file_path, file_size)); 

	
	// Code for Random scheduling 
	if (!load_balance && file_task_queue[turn].cnt >= TASK_QUEUE_FULL)
	//if (!load_balance && file_task_queue[turn].cnt == TASK_QUEUE_FULL)
	{
		// Code for random scheduling
		int rc = MPI_Send(file_task_queue[turn].queue.c_str(), file_task_queue[turn].queue.length()+1, MPI_CHAR, turn % (worldSize-1) + 1, 1, MPI_COMM_WORLD); 
		//int rc = MPI_Send(file_task_queue[turn].queue.c_str(), file_task_queue[turn].queue.length()+1, MPI_CHAR, turn % (worldSize-1) + 1,TASK_QUEUE_FULL, MPI_COMM_WORLD); 

		if (rc != MPI_SUCCESS)
			cout << "MPI Send failed\n"; 

		file_task_queue[turn].queue.clear(); 
		
		// Random Scheduling: it should be uncommented
		//file_task_queue[turn].cnt = 0; 
	}
	
	// it should be uncommented back 
	//turn = (turn + 1) % (worldSize-1);

	// Random Scheduling
	do{
		turn = rand() % workerNumber; 
	}while (file_task_queue[turn].cnt == MAX_FILE_NUM_PER_WORKER  &&  task_remaining_workers != 0);
	
	/*
	turn: 워커 idx (0~workerNumber-1)
	cnt: 해당 워커에게 분배한 파일작업의 개수
	모든 file_task_queue에게 배분한 파일 작업의 개수가 6개로 같으면 (96*6=576) 마스터는 종료해야 함.


	*/

}


void Dedupe:: file_task_load_balance(vector<pair<string, uint64_t>> & file_task_list)
{
	auto compareTotalFileSize =
			[](const File_task_queue* q1, File_task_queue* q2) {
					        return q1->total_file_size > q2->total_file_size; 
							// ">" for ascending order, "<" for descending order
			};

	
	priority_queue<File_task_queue*, vector<File_task_queue*>, decltype(compareTotalFileSize)> rank_allocator(compareTotalFileSize);
	for (int i = 0; i < worldSize-1; i ++)
	{
		File_task_queue * q = new File_task_queue; 
		q->cnt = i+1; 
		// q->cnt = 0; 
		q->total_file_size = 0; 
		
		rank_allocator.push(q); 
	}

	sort(file_task_list.begin(), file_task_list.end(), [](auto &left, auto &right){
		return left.second > right.second; 
	}); 


	for (auto file_task: file_task_list)
	{
		auto file_task_queue = rank_allocator.top(); 
		rank_allocator.pop();
	
		printf("%lld ", file_task.second); 

		// file_task_queue->queue += file_task.first;
		file_task_queue->total_file_size += file_task.second; 
		// file_task_queue->cnt += 1; 
		rank_allocator.push(file_task_queue); 
	
		int rc = MPI_Send(file_task.first.c_str(), file_task.first.length()+1, MPI_CHAR, file_task_queue->cnt, 1, MPI_COMM_WORLD); 
		// int rc = MPI_Send(file_task_queue->queue.c_str(), file_task_queue->queue.length()+1, MPI_CHAR, file_task_queue->cnt, 1, MPI_COMM_WORLD); 
		if (rc != MPI_SUCCESS)
			cout << "MPI Send failed\n"; 	

	}
	printf("\n"); 


	cout << "hello world\n"; 

	for (int i = 0; i < worldSize-1; i++)
	{
		auto file_task_queue = rank_allocator.top();
		rank_allocator.pop(); 
		printf("rank %d\t%lld\n", file_task_queue->cnt, file_task_queue->total_file_size); 
		// cout << "file size sent to rank: " << file_task_queue->total_file_size  << endl ; 
	// 	int rc = MPI_Send(file_task_queue->queue.c_str(), file_task_queue->queue.length()+1, MPI_CHAR, i+1,file_task_queue->cnt, MPI_COMM_WORLD); 
	// 	if (rc != MPI_SUCCESS)
	// 		cout << "MPI Send failed\n"; 		
	}



	









}

// Load Balancing code
void  Dedupe:: object_task_load_balance(vector<vector<object_task*>>& task_queue)
{
	auto compareSecondElement =
			[](const std::pair<int, uint64_t>& p1, const std::pair<int, uint64_t>& p2) {
					        return p1.second > p2.second; 
							// ">" for ascending order, "<" for descending order
			};
	priority_queue<pair<int, uint64_t>, vector<pair<int, uint64_t>>, decltype(compareSecondElement)> total_size_per_rank(compareSecondElement);
	
	for (int i = 1; i < worldSize; i++)
	{
		auto p = make_pair(i, 0);
		total_size_per_rank.push(p);
	}
	sort(this->size_per_ost, this->size_per_ost + OST_NUMBER, greater<uint64_t>());
	
	int task_num;
		  
			 // Distribute tasks from each OST to the rank whose size is minimal. 
	for (int i = 0; i < OST_NUMBER; i++)
	{
		// Get the rank which contains least size of tasks.
		pair<int, uint64_t> p = total_size_per_rank.top();
		total_size_per_rank.pop();
		int rank = p.first;

		char * Msg = object_task_queue_clear(task_queue[i], &task_num);
		
		printf("task_num: %d\n", task_num); 
		int rc = MPI_Send(Msg, sizeof(object_task) * task_num, MPI_CHAR, rank, task_num, MPI_COMM_WORLD);
		if (rc != MPI_SUCCESS)
			cout << "MPI Send Failed\n";

		p.second += this->size_per_ost[i];
		total_size_per_rank.push(p); 
	}
	
	// Test Code 
	/*
	printf("per ost\n"); 
	for(int i = 0; i < OST_NUMBER; i++)
	{
		printf("ost\t%d\t%lld\n", i, size_per_ost[i]); 
	}


	 printf("Before Load Balance\n");
	 for (int i = 1; i <= this->worldSize-1; i++)
	{
		printf("rank\t%dsize\t%llu\n", i, this->size_per_rank[i]);
	}
	
	 printf("After Load Balance\n"); 
	 for(int i = 0; i < this->worldSize-1; i++)
	 {
		pair<int, uint64_t> p = total_size_per_rank.top();
		total_size_per_rank.pop();
										
		printf("rank\t%d\tsize\t%llu\n", p.first, p.second);
	 }// test purpose code
	*/

}

char * Dedupe::object_task_queue_clear(vector<object_task*> &task_queue, int *task_num){
	
	char * Msg = new char[sizeof(object_task) * task_queue.size()]; 
	//char * Msg = new char[sizeof(object_task) * TASK_QUEUE_FULL];
	if (Msg == nullptr){
		cout << "Memory allocation error\n"; 
		return nullptr;
	}

	if (task_num != nullptr)
		*task_num = task_queue.size(); 

	char* buffer = new char[sizeof(object_task)];
	if(buffer == nullptr){
		cout << "Memory allocation error\n";
		return nullptr;
	}	

	int idx = 0; 
	while (!task_queue.empty()){
		object_task * task = task_queue.back(); 
		task_queue.pop_back(); 

		object_task_serialization(task, buffer); 
		Msg_Push(buffer, Msg, idx); 
		idx ++; 
	}
	delete[] buffer;

	return Msg; 
}

void Dedupe::object_task_buffer_free (char * buffer){
	delete[] buffer; 
}

// Serialize struct OBJECT_TASK into string so that it can be sent by MPI_SEND command.
// Each element is delimited by ,(comma) and ENDOF struct is marked by '\n'
void Dedupe:: object_task_serialization(object_task* task, char * buffer)
//string  object_task_serialization(struct object_task* task)
{
	int filepath_len = strlen(task->file_path); 

	int * p = (int *)buffer; 
	*p = task->ost; p++; 
	
	uint64_t *q = (uint64_t*)p; 
	*q = task->start; q++; 
	*q = task->end; q++; 
	*q = task->interval; q++; 
	*q = task->size; q++; 

	char *r = (char*)q; 
	for (int i = 0; i < filepath_len; i ++)
	{
		*r = task->file_path[i]; 
		r ++; 
	}
	*r = 0; 
	
}

object_task * Dedupe::object_task_deserialization(const char* buffer){
	object_task * task = new object_task; 
	//const char * buf = buffer.c_str()d`; 
	
	int *p = (int *)buffer;
	task->ost = *p; p ++; 

	uint64_t *q = (uint64_t*)p; 
	task->start = *q; q++; 
	task->end = *q; q++; 
	task->interval = *q; q++; 
	task->size = *q; q++; 

	memcpy(task->file_path, q, MAX_FILE_PATH_LEN); 
	//string file_path((char*)q);  
	//task->file_path = file_path; 


	//printf("%d %d %d %d \n", task->ost, task->start, task->end, task->interval) ; 
	//cout << task->file_path << "\n" << "hello world!";

	return task; 
}

void Dedupe::object_task_insert(object_task* task, vector<object_task*> queue){
	queue.push_back(task); 	
	return; 
}

object_task* Dedupe::object_task_generate(const char * file_path, int ost, uint64_t start, uint64_t end, uint64_t interval, uint64_t size){
	// task should be deleted after completing the task on Reader process. 
	object_task *task = new object_task; 
	
	//memcpy(task->file_path, file_path, MAX_FILE_PATH_LEN); 
	memcpy(task->file_path, file_path, strlen(file_path)+1); 
	//task->file_path = file_path; 
	//
	task->ost = ost; 
	task->start = start; 
	task->end = end; 
	task->interval = interval; 
	task->size = size; 

	return task; 
}

};

