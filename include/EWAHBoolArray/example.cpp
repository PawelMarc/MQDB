#include <stdlib.h>
#include "ewah.h"

#define LENGTH uword64

int main(void)  {
    EWAHBoolArray<LENGTH> bitset1;
    EWAHBoolArray<LENGTH> bitset2;
    EWAHBoolArray<LENGTH> orbitset;
    
    bitset1.set(1);
        
    bitset2.set(2);

    bitset1.logicalor(bitset2, orbitset);
    
    cout<<"logical or: "<<endl;
    for(EWAHBoolArray<LENGTH>::const_iterator i = orbitset.begin(); i!=orbitset.end(); ++i)
        cout<<*i<<endl;
    cout<<endl;
    
    return 0;
}

#if 0
int main(void) {
    EWAHBoolArray<LENGTH> bitset1;
    EWAHBoolArray<LENGTH> bitset2;
    EWAHBoolArray<LENGTH> orbitset;
    EWAHBoolArray<LENGTH> andbitset;
    EWAHBoolArray<LENGTH> notbitset;
    
    for (int i =0; i<1000000; i++) {
    //for (int i =0; i<1; i++) {
        bitset1.reset();
        bitset2.reset();
        orbitset.reset();
        andbitset.reset();
        notbitset.reset();
    
        bitset1.set(1);
        
        
        bitset1.set(2);
        bitset1.set(10);
        bitset1.set(1000);
        bitset1.set(1001);
        bitset1.set(1002);
        bitset1.set(1003);
        bitset1.set(1007);
        bitset1.set(1009);
        bitset1.set(100000);
        bitset1.set(10000000);
        
        bitset2.set(1);
        bitset1.makeSameSize(bitset2);
        
        //bitset2.set(3);
        //bitset2.set(1000);
        //bitset2.set(1007);
        //bitset2.set(100000);
        //bitset2.set(10000000-1);
        
        //bitset2.logicalnot(notbitset);
        bitset2.inplace_logicalnot();
        
        //bitset1.logicalor(bitset2,orbitset);
        //bitset1.sparselogicaland(notbitset,andbitset);
        bitset1.logicaland(bitset2,andbitset);
    }

            
    cout<<"first bitset : "<<endl;
    for(EWAHBoolArray<LENGTH>::const_iterator i = bitset1.begin(); i!=bitset1.end(); ++i)
        cout<<*i<<endl;
    cout<<endl;

/*
    cout<<"second bitset : "<<endl;
    for(EWAHBoolArray<LENGTH>::const_iterator i = bitset2.begin(); i!=bitset2.end(); ++i)
        cout<<*i<<endl;
    cout<<endl;
*/
  
    // we will display the or
    cout<<"logical and: "<<endl;
    for(EWAHBoolArray<LENGTH>::const_iterator i = andbitset.begin(); i!=andbitset.end(); ++i)
        cout<<*i<<endl;
    cout<<endl;
    cout<<"memory usage of compressed bitset = "<<andbitset.sizeInBytes()<<" bytes"<<endl;
    cout<<endl;
    // we will display the and
    cout<<"logical or: "<<endl;
    for(EWAHBoolArray<LENGTH>::const_iterator i = orbitset.begin(); i!=orbitset.end(); ++i)
        cout<<*i<<endl;
    cout<<endl;
    cout<<endl;
    cout<<"memory usage of compressed bitset = "<<orbitset.sizeInBytes()<<" bytes"<<endl;
    cout<<endl;
    
    
    return EXIT_SUCCESS;
}

#endif

