#include <mpi.h>
#include <cstdio>
#include <vector>
#include <Windows.h>

#include "LONG_DEC.h"

//#define USE_MASTER_SLAVE

typedef void (*Series_Method)(INT64, LONG_DEC&, LONG_DEC&);

void test_res(const char* path, LONG_DEC& res, int print_len, int n_iter, int n_thread, double ts);

void leibniz_term(INT64 n_term, LONG_DEC& res, LONG_DEC& oprd){
	// pi/4 = 1 - 1/3 + 1/5 - 1/7 + ...
	// by default, n_item starts from 0;
	oprd = 4;
	oprd.int_div((n_term << 1) + 1);
	if (n_term & 0x1) res += oprd;
	else res -= oprd;
	return;
}

void the_unknown_series_term(INT64 n_term, LONG_DEC& res, LONG_DEC& oprd) {
	// pi/2 = 1 + 1 * 1/3 + 1 * 1/3 * 2/5 + 1 * 1/3 * 2/5 * 3/7 + ...+ 1 * ... * n/(2*n+1)
	oprd = 2;
	for (INT64 i = 1; i <= n_term; ++i) {
		oprd.int_mul(i);
		oprd.int_div((i << 1) + 1);
	}
	res += oprd;
	return;
}

struct Message {
	int field[2]{};
};

enum TAG : int {
	// sending from master;
	cont_calc = 6, // send index;
	send_back_res = 99,
	// sending from slaves;
	calc_done = 9,
	sending_res = 66,
};

inline bool check_all_free(bool* b, int len) {
	// Do NOT check master proc;
	for (int i = 1; i < len; ++i)
		if (b[i] == true)
			return false;
	return true;
}

inline void scale_down_batch(int& bs, float f, int minsz) {
	if (bs > minsz) {
		bs = static_cast<int>(bs * f);
		bs = ((bs < minsz) ? minsz : bs);
	}
}

int convert_num(const char* str);


int main(int argc, char** argv)
{

	int n_iter = 4000;

	bool use_average_assign = true;

	bool do_batch_scaling = true;
	constexpr int min_batch_size = 5;
	constexpr float scaling = 0.9;

	Series_Method use_method = the_unknown_series_term;
	LARGE_INTEGER t0, t1, fq;

	// =================================================

	if (argc == 2) {
		n_iter = convert_num(argv[1]);
	}
	if (argc == 3) {
		n_iter = convert_num(argv[1]);
		if (strcmp(argv[2], "--no_scaling") == 0) {
			do_batch_scaling = false;
		}
		else if (strcmp(argv[2], "--dynamic") == 0) {
			use_average_assign = false;
		}
	}
	if (argc == 4) {
		n_iter = convert_num(argv[1]);
		if (strcmp(argv[2], "--no_scaling") == 0) {
			do_batch_scaling = false;
		}
		if (strcmp(argv[3], "--dynamic") == 0) {
			use_average_assign = false;
		}
	}

	// =================================================

	int my_rank;
	int world_size;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	if (my_rank == 0) {
		QueryPerformanceFrequency(&fq);
		QueryPerformanceCounter(&t0);
	}

	// ======================== Computation phase ===========================

	LONG_DEC operand, local_res;
	MPI_Status status{};
	Message message;

#ifdef USE_MASTER_SLAVE

	if (my_rank == 0) {
		bool busy[64]{};
		memset(busy, 0, sizeof(busy));
		busy[0] = true;

		int batch_size = n_iter / world_size / 2;

		int assigned = 0;
		int probe_flag = 0;

		//std::vector<int> fibo_list;
		//{
		//	int ss = 0;
		//	int last2 = 0;
		//	int last = 1;
		//	int cur = 0;
		//	while (ss < n_iter) {
		//		cur = last + last2;
		//		fibo_list.push_back(cur);
		//		ss += cur;
		//		last2 = last;
		//		last = cur;
		//	}
		//	fibo_list.pop_back();
		//}
		

		while (true) {

			// master exit
			if (check_all_free(busy, world_size) && (assigned >= n_iter)) {
				
				// before exiting, imform each slave to send back results;
				for (int i = 1; i < world_size; ++i) {
					MPI_Send(&message, _countof(message.field), MPI_INT32_T, i, TAG::send_back_res, MPI_COMM_WORLD);
				}
				break;
			}

			// assignment loop
			for (int i = 1; i < world_size; ++i){
				if (busy[i]) continue;
				
				if (assigned >= n_iter) break;

				// use fibonacci array to assign batch_size;
				//if (!fibo_list.empty()) {
				//	batch_size = fibo_list.back();
				//	fibo_list.pop_back();
				//	if (batch_size < min_batch_size)
				//		batch_size = min_batch_size;
				//}

				if (use_average_assign) {
					message.field[0] = i - 1;
					message.field[1] = n_iter;
				}
				else {
					message.field[0] = assigned;
					assigned += batch_size;
					if (assigned >= n_iter)	assigned = n_iter;
					message.field[1] = assigned;
				}

				busy[i] = true;
				// if busy[i] is false, slave i is waiting for the next command;
				MPI_Send(&message, _countof(message.field), MPI_INT32_T, i, TAG::cont_calc, MPI_COMM_WORLD);

				// every time assigned a task, scaling down the batch_size;
				scale_down_batch(batch_size, scaling, min_batch_size);

				//printf("[master] Work [%d, %d) has been assigned to proc %d, batch_sz=%d.\n", message.field[0], message.field[1], i, batch_size);
			}

			if (use_average_assign) assigned = n_iter;

			// check status of each slave
			for (int i = 1; i < world_size; ++i) {

				// at this phase, slave could only send back "calc_done" tag;
				MPI_Iprobe(i, TAG::calc_done, MPI_COMM_WORLD, &probe_flag, &status);

				if (probe_flag) {
					probe_flag = 0;

					// TODO: need to check status?

					busy[i] = false;
					MPI_Recv(&message, _countof(message.field), MPI_INT32_T, i, TAG::calc_done, MPI_COMM_WORLD, &status);

				}
			}

		}

	}
	else {

		int step = 1;
		if (use_average_assign) {
			// actually only (world_size - 1) processes are doing the job;
			step = world_size - 1;
		}

		while (true) {

			// slave must not take any action unless receiving command from the master;
			// TODO: but master should not let slaves wait for too long;
			MPI_Recv(&message, _countof(message.field), MPI_INT32_T, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			if (status.MPI_TAG == TAG::cont_calc) {

				int& start = message.field[0];
				int& end = message.field[1];


				// ======================== extensive computation ========================
				for (int i = start; i < end; i += step) {
					use_method(i, local_res, operand);
				}
				// ======================== extensive computation ========================

				// slave must report to the master that he is done with computation;
				MPI_Send(&message, _countof(message.field), MPI_INT32_T, 0, TAG::calc_done, MPI_COMM_WORLD);

			}
			else if (status.MPI_TAG == TAG::send_back_res) {

				break;
			}
			else {
				//error happened;

				break;
			}
		}
	}
#else
	
	// do not use master-slave model, direct averange assigning.
	int index = 0;

	for (index = my_rank; index < n_iter; index += world_size) {
		use_method(index, local_res, operand);
	}

#endif
	// ======================== Data collection phase ===========================

	if (my_rank == 0) {

		for (int i = 1; i < world_size; ++i) {
			// This is a blocking receive, which will wait for a sender;
			MPI_Recv(operand._buff(), LONG_DEC::ARR_SIZE, MPI_UNSIGNED_CHAR, i, TAG::sending_res, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			local_res += operand;
		}

	}
	else {
		MPI_Send(local_res._buff(), LONG_DEC::ARR_SIZE, MPI_UNSIGNED_CHAR, 0, TAG::sending_res, MPI_COMM_WORLD);
	}

	// ======================== result testing phase ===========================

	if (my_rank == 0) {
		QueryPerformanceCounter(&t1);
		double ts = double(t1.QuadPart - t0.QuadPart) / fq.QuadPart;

		test_res("D:/Projects/MS-MPI/learn_MPI/pi_exact.txt", local_res, 16, n_iter, world_size, ts);
	}

	MPI_Finalize();

	return 0;
}


void test_res(const char* path, LONG_DEC& res, int print_len, int n_iter, int n_thread, double ts) {
	FILE* f;
	fopen_s(&f, path, "rt");
	if (f == NULL) {
		printf("pi_exact.txt opening failed.\n");
		return;
	}

	char read_specifier[7] = "%0000s";
	int dvd = LONG_DEC::ARR_SIZE;
	int i = 4;
	while (i >= 1 && dvd != 0) {
		read_specifier[i--] = char(dvd % 10) + '0';
		dvd /= 10;
	}

	char inbuff[LONG_DEC::ARR_SIZE + 10]{};
	int scanf_flag = fscanf_s(f, read_specifier, inbuff, (unsigned)_countof(inbuff));
	fclose(f);

	if (scanf_flag <= 0) {
		printf("Error: fscanf_s() failed to read.\n");
		return;
	}

	int counter = (res == inbuff);
	if (counter > LONG_DEC::UNIT_DIG) counter -= LONG_DEC::UNIT_DIG;
	else counter = 0;

	printf("The length of array: %d\n", LONG_DEC::ARR_SIZE);
	printf("Number of iteration : %d\n", n_iter);
	printf("Number of thread : %d\n", n_thread);
	printf("Precision goes to %d digits\n", counter);
	printf("Time usage: %.3fsec\n", ts);
	printf("Approxixmation:\n");
	res.print();
	printf("\n");
	return;
}

int convert_num(const char* str) {
	int dig = 0;
	while (str[dig]) ++dig;
	--dig;
	int res = 0;
	int unit = 1;
	while (dig >= 0) {
		res += (str[dig] - '0') * unit;
		unit *= 10;
		--dig;
	}
	return res;
}