#include <iostream>
#include <string>
#include <exception>

#include "mem_res.hpp"  
#include "queue.hpp"  

int main() {
    try {
        StaticVectorBlocks pool(64 * 1024);

        PmrQueue<int> qi(4, &pool); 
        qi.push(1);
        qi.push(2);
        qi.emplace(3);
        qi.push(4);

        std::cout << "int очередь (итерация): ";
        for (auto it = qi.begin(); it != qi.end(); ++it) {
            std::cout << *it << ' ';
        }
        std::cout << "\nразмер: " << qi.size() << '\n';

        while (!qi.empty()) {
            std::cout << "pop int: " << qi.front() << '\n';
            qi.pop();
        }


        struct Complex {
            int id;
            double val;
            std::pmr::string name;

            Complex(int i, double v, std::pmr::string n)
                : id(i), val(v), name(std::move(n)) {}
        };

        PmrQueue<Complex> qc(2, &pool);

        std::pmr::string s1("alpha", &pool);
        std::pmr::string s2("beta_long_name", &pool);

        qc.emplace(10, 3.14, s1);
        qc.emplace(11, 2.71, s2);
        qc.emplace(12, 1.41, std::pmr::string("gamma", &pool)); 

        std::cout << "\nочередь структур (итерация):\n";
        for (auto it = qc.begin(); it != qc.end(); ++it) {
            const Complex& c = *it;
            std::cout << "  id=" << c.id << " val=" << c.val << " name=" << c.name << '\n';
        }

        while (!qc.empty()) {
            const Complex& c = qc.front();
            std::cout << "pop complex id=" << c.id << " name=" << c.name << '\n';
            qc.pop();
        }

    } catch (const std::bad_alloc&) {
        std::cerr << "Ошибка: пул памяти исчерпан (std::bad_alloc)\n";
        return 2;
    } catch (const std::exception& ex) {
        std::cerr << "Исключение: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
