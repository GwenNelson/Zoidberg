extern write(int fd, const void *buf, int count);

int main() {
    write(1,"Init\n",5);
}
