int main(void) {
    int n = 10;
    int total = 0;

    for (int i = 0; i < n; i = i + 1) {
        total = total + i;
    }

    for (int j = 0; j < n; j = j + 1) {
        total = total + j;
    }

    return total;
}