int main(void) {
    int n = 5;
    int arr[5];
    int dupes = 0;

    for (int i = 0; i < n; i++) {
        arr[i] = i * 2;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j += 1) {
            if (arr[i] == arr[j]) {
                dupes++;
            }
        }
    }

    return dupes;
}