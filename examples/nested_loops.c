// for is mentioned here, but this is not a loop

int main(void) {
    int n = 10;
printf("the word for is not a loop\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int result = i + j;
        }
    }

    return 0;
}
