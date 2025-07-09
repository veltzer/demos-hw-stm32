

void task_a() {
	while(1) {
		do_some_work_for_a();
		save_register to task_a_last_state and jmp to task_b_last_state
		...
	}
}

void task_b() {
	while(1) {
		do_some_work_for_b();
		save_register to task_b and jmp to task_a_last_state
		...
	}
}
