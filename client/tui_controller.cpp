#include <stack>
#include "screen.cpp"

class TuiController {
    private:
    TuiController(){

    }
    public:
    TuiController(const TuiController&) = delete;
    TuiController& operator=(const TuiController&) = delete;

    static TuiController* getTuiController(){
        static TuiController controller;
        return &controller;
    }
    
    void run(){
        auto login = new Login();
        auto _n = login->display();
        
        std::stack<Screen*> navStack;
        navStack.push(_n);

        while(!navStack.empty()){
            Screen* current = navStack.top();
            navStack.pop();
            Screen* next = current->display();
            system("clear");
            if(next != NULL){
                navStack.push(current);
                navStack.push(next);
            } else {
                delete current;
            }
        }
        pthread_exit(NULL);
    }
};