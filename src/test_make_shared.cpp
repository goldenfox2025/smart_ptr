#include <iostream>
#include <string>
#include "control_.hpp"

struct TestObject {
    int value;
    std::string name;
    
    TestObject(int v, const std::string& n) : value(v), name(n) {
        std::cout << "TestObject constructed with value=" << value << ", name=" << name << std::endl;
    }
    
    ~TestObject() {
        std::cout << "TestObject destroyed with value=" << value << ", name=" << name << std::endl;
    }
};

int main() {
    std::cout << "Testing make_shared function..." << std::endl;
    
    // Test with multiple arguments
    {
        std::cout << "Creating shared pointer with make_shared..." << std::endl;
        auto ptr = ::make_shared<TestObject>(42, "test");
        
        std::cout << "Accessing shared object: value=" << ptr->value << ", name=" << ptr->name << std::endl;
        
        std::cout << "Creating another reference..." << std::endl;
        auto ptr2 = ptr;
        
        std::cout << "First reference going out of scope..." << std::endl;
    }
    
    std::cout << "All references gone, object should be destroyed." << std::endl;
    
    return 0;
}
