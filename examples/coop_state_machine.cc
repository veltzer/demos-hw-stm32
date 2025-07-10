

bool task1(task1state) {
	// do 1 step in the computation starting from task1state
	// return false when done
}

bool task2(task2state) {
	// do 1 step in the computation starting from task2state
	// return false when done
}

void main() {
	initalize task1state
	initalize task2state
	while(1) {
		task1(task1state);
		task2(task2state);
	}
}
