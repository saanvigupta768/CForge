int main(void) {
    int n = 5;
    int count = 0;

    for (int i = 0; i < n; i = i + 1) {
        for (int j = 0; j < n; j = j + 1) {
            for (int k = 0; k < n; k = k + 1) {
                count = count + 1;
            }
        }
    }

    return count;
}