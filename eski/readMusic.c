#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define LENGTH 4

int main() {
    FILE *fp;
    fp = fopen("sample.bin", "rb");
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    printf("%d\n",sz);
    rewind(fp);
    int16_t music[LENGTH];
    printf("%d bytes each \n", sizeof(music[0]));
    int count;
    count = 0;
    while(sz-count > LENGTH){
      fread(music, 2, LENGTH, fp);
      for (int x = 0; x < LENGTH; x++) {
          printf("%d ", music[x]);
      }
      printf("\n");
      //printf("\n%d COUNT\n", count);
      count += LENGTH;
      //fseek(fp, 0, LENGTH);
    }



    return (0);
}
