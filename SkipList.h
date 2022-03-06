#ifndef KVSTORAGE_SKIPLIST_H_  
#define KVSTORAGE_SKIPLIST_H_

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dump_file"
std::mutex Mutex; // 操纵数据时使用
std::string delimiter = ":"; // 数据分隔符，加载数据时会用到

// 跳表节点结构
template<typename K, typename V>
class SkipListNode {
public:
    SkipListNode() {};
    SkipListNode(K k, V v, int);
    ~SkipListNode();

    K get_key() const {
        return key;
    }

    V get_value() const {
        return value;
    }

    void set_value(V); 

    SkipListNode<K, V> **forward;

    int SkipListNode_level;  // 当前节点的最大层数
private:
    K key;  
    V value;
};

template<typename K, typename V>
SkipListNode<K, V>::SkipListNode(K k, V v, int level) : key(k), value(v), SkipListNode_level(level) {
    forward = new SkipListNode<K, V>*[SkipListNode_level + 1];

    memset(forward, 0, sizeof(SkipListNode<K, V>*) * (level + 1));
}

template<typename K, typename V>
SkipListNode<K, V>::~SkipListNode() {
    delete[] forward;
}

template<typename K, typename V>
void SkipListNode<K, V>::set_value(V v) {
    value = v;
}


// 跳表结构
template<typename K, typename V>
class SkipList {
public:
    SkipList(int);
    ~SkipList();

    int get_random_level();
    SkipListNode<K, V>* create_SkipListNode(K, V, int);

    int insert_element(K, V);  // 插入数据
    bool find_element(K);      // 查找数据
    void display_list();       // 展示数据
    void delete_element(K);   // 删除数据
    void dump_file();          // 数据落盘
    void load_file();          // 从文件加载数据
    int size();                // 返回数据规模
private:
    void get_KeyValue_from_string(const std::string& s, std::string* key, std::string* value); // 从加载的数据中获取key和value

    bool is_valid_string(const std::string& s); // 判断从文件读取的字符串是否有效（是否为空，是否含有分隔符）


    int max_level;             // 跳表的最大层数
    int SkipList_level;        // 当前跳表中层数最大的节点层数

    SkipListNode<K, V>* head;          // 指向跳表头节点

    std::ofstream file_writer;
    std::ifstream file_reader;

    int element_count;         // 跳表当前元素个数
};

template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) : max_level(max_level), SkipList_level(0), element_count(0) {
    K k;
    V v;
    head = new SkipListNode<K, V>(k, v, max_level);
}

template<typename K, typename V>
SkipList<K, V>::~SkipList() {
    if(file_writer.is_open()) {
        file_writer.close();
    }

    if(file_reader.is_open()) {
        file_reader.close();
    }

    delete head;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level() {
    int k = 1;

    while(rand() % 2) {
        ++k;
    }

    if(k > max_level)
        k = max_level;
    return k;
}

template<typename K, typename V>
SkipListNode<K, V>* SkipList<K, V>::create_SkipListNode(K k, V v, int level) {
    SkipListNode<K, V>* new_SkipListNode = new SkipListNode<K, V>(k, v, level);
    return new_SkipListNode;
}

/*
插入数据（默认不允许插入重复的key）
返回1表示元素已经存在
返回0表示成功插入
*/
template<typename K, typename V>
int SkipList<K, V>::insert_element(K key, V value) {
    // 增加数据时，首先需要上锁
    Mutex.lock();

    SkipListNode<K, V>* current = head;

    // 首先创建更新数组，数组中存放的是需要更新前进指针forward的节点
    // update[i]就是需要更新第i层前进指针的节点
    SkipListNode<K, V>* update[max_level + 1];
    memset(update, 0, sizeof(SkipListNode<K, V>*) * (max_level + 1));

    // 从当前的最高层数开始遍历，找到当前层小于待插入key的最大元素（插入新元素后，当前找到的节点的这一层的前进指针应该指向新插入的元素），将找到的节点放到对应层的update数组中
    // 当前层的节点找到后，只需要 从当前节点开始 继续寻找下一层的节点即可
    for(int i = SkipList_level; i >= 0; --i) {
        while(current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // 经过上面的遍历，此时已经到了第0层，而current指向的节点是所有节点中小于待插入key的最大的节点
    // 换句话说，待插入节点应该插到current的右边
    // 所以将current右移一个节点，对其进行判断
    current = current->forward[0];
    if(current != NULL && current->get_key() == key) {
        std::cout << "key:  " << key << "  already exists" << std::endl;
        Mutex.unlock();
        return 1;
    }

    // 如果移动后current为NULL，那么需要在当前位置插入新节点
    // 如果移动后current的key大于待插入的key，那么需要在update[0]和current之间插入新节点
    if(current == NULL || current->get_key() != key) {
        // 首先需要获得一个随机层数
        int random_level = get_random_level();

        // 如果这个随机层数大于 当前 跳表的最大层数，那么头节点的前进指针也需要更新，更新的层数范围是[当前跳表的最大层数 + 1, 随机层数]，而由于修改的都是头节点，所以update数组中也都记录的是头节点
        if(random_level > SkipList_level) {
            for(int i = SkipList_level + 1; i <= random_level; ++i) {
                update[i] = head;
            }
            SkipList_level = random_level; // 更新当前最大层数
        }

        // 创建一个具有随机层数的节点
        SkipListNode<K, V>* insert_node = create_SkipListNode(key, value, random_level);

        // 插入节点（修改每一层前进指针的指向即可）
        for(int i = 0; i <= random_level; ++i) {
            insert_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = insert_node;
        }

        std::cout << "Successfully insert key: " << key << " value: " << value << std::endl;
        ++element_count;
    }
    Mutex.unlock();
    return 0;
}

// 删除数据
template <typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
    // 删除数据前需要加锁
    Mutex.lock();

    SkipListNode<K, V>* current = head;
    // 与插入数据时一样需要构建update数组
    // update数组的大小只需和当前跳表的最高层数一样即可，因为待删除节点的层数不会大于当前跳表的最高层数
    SkipListNode<K, V>* update[SkipList_level + 1];
    memset(update, 0, sizeof(SkipListNode<K, V>*) * (SkipList_level + 1));

    for(int i = SkipList_level; i >= 0; --i) {
        while(current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // 遍历完第0层时，需要取current右边的节点进行Key的判断
    current = current->forward[0];
    if(current != NULL && current->get_key() == key) {
        // 通过update数组更新数组中每个节点的前进指针
        for(int i = 0; i <= current->SkipListNode_level; ++i) {
            update[i]->forward[i] = current->forward[i];
        }

        // 还需要对当前跳表的最大层数进行更新
        // 利用头节点进行更新，如果头节点的第 i 层的前进指针为NULL，说明当前跳表的最大层数小于 i 
        while(SkipList_level > 0 && head->forward[SkipList_level] == NULL) {
            --SkipList_level;
        }

        std::cout << "Successfully delete key: " << key << std::endl;
        --element_count;
        delete current;
    }

    Mutex.unlock();
    return;
}

// 查找数据
template<typename K, typename V>
bool SkipList<K, V>::find_element(K key) {
    SkipListNode<K, V>* current = head;

    for(int i = SkipList_level; i >= 0; --i) {
        while(current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];
    if(current != NULL && current->get_key() == key) {
        std::cout << "Successfully find key: " << key << " value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not find key: " << key << std::endl;
    return false;
}


// 展示数据
template<typename K, typename V>
void SkipList<K, V>::display_list() {
    std::cout << "\n------Skip List------\n";
    for(int i = SkipList_level; i >= 0; --i) {
        SkipListNode<K, V>* Node = head->forward[i];
        std::cout << "Level " << i << ": ";
        while(Node) {
            std::cout << "Key: " << Node->get_key() << " Value: " << Node->get_value() << ";";
            Node = Node->forward[i];
        }
        std::cout << std::endl;
    }
}

// 数据落盘
template<typename K, typename V>
void SkipList<K, V>::dump_file() {
    std::cout << "\n------Dump File------\n";

    file_writer.open(STORE_FILE);

    SkipListNode<K, V>* Node = head->forward[0];
    while(Node) {
        file_writer << Node->get_key() << ":" << Node->get_value() << "\n"; // 方便读出数据
        std::cout << "Key: " << Node->get_key() << " Value: " << Node->get_value() << "\n";
        Node = Node->forward[0];
    }

    file_writer.flush();
    file_writer.close();
    return;
}

// 从文件加载数据
template<typename K, typename V>
void SkipList<K, V>::load_file() {
    file_reader.open(STORE_FILE);
    std::cout << "\n------Loading------\n";
    
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while(getline(file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if(key->empty() || value->empty()) {
            continue;
        }

        insert_element(*key, *value);
        std::cout << "Key: " << *key << "Value: " << *value << std::endl;
    }
    file_reader.close();
    delete key;
    delete value;
}

// 返回数据规模
template<typename K, typename V>
int SkipList<K, V>::size() {
    return element_count;
}

// 从加载的数据（string）中获得key和value
template<typename K, typename V>
void SkipList<K, V>::get_KeyValue_from_string(const std::string& s, std::string* key, std::string* value) {
    if(!is_valid_string(s)) {
        return;
    }
    *key = s.substr(0, s.find(delimiter));
    *value = s.substr(s.find(delimiter) + 1);
}

// 判断从文件读取的字符串是否有效（是否为空，是否含有分隔符）
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& s) {
    if(s.empty() || s.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

#endif  // KVSTORAGE_SKIPLIST_H