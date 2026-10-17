int main(void) {
    int n = 100;
    int work = 0;

    for (int i = 0; i < n; i = i + 1) {
        for (int j = 1; j < n; j = j * 2) {
            work = work + 1;
        }
    }

    return work;
}