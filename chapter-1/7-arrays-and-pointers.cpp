
void array_stack() {
    int stack[10];
}

void array_heap() {
    int* heap = new int[10];
    
    // delete[] heap;
    delete heap;
}

int main() {
    array_heap();
    // array_stack();

    return 0;
}