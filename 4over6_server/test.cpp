#include <iostream>
#include <string.h>
#include <time.h>
#include <unistd.h>
using namespace std;
int main(){
while(1){

unsigned int timer = time(NULL);
cout<<timer<<endl;
usleep(1000000);

}
return 0;
}
