int main(void) {
    int n = 100;
    int steps = 0;

    for (int i = 1; i < n; i = i * 2) {
        steps = steps + 1;
    }

    return steps;
}