#ifndef KVSTORAGE_SKIPLIST_H_  
#define KVSTORAGE_SKIPLIST_H_

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dump_file"
std::mutex Mutex; // ��������ʱʹ��
std::string delimiter = ":"; // ���ݷָ�������������ʱ���õ�

// ����ڵ�ṹ
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

    int SkipListNode_level;  // ��ǰ�ڵ��������
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


// ����ṹ
template<typename K, typename V>
class SkipList {
public:
    SkipList(int);
    ~SkipList();

    int get_random_level();
    SkipListNode<K, V>* create_SkipListNode(K, V, int);

    int insert_element(K, V);  // ��������
    bool find_element(K);      // ��������
    void display_list();       // չʾ����
    void delete_element(K);   // ɾ������
    void dump_file();          // ��������
    void load_file();          // ���ļ���������
    int size();                // �������ݹ�ģ
private:
    void get_KeyValue_from_string(const std::string& s, std::string* key, std::string* value); // �Ӽ��ص������л�ȡkey��value

    bool is_valid_string(const std::string& s); // �жϴ��ļ���ȡ���ַ����Ƿ���Ч���Ƿ�Ϊ�գ��Ƿ��зָ�����


    int max_level;             // �����������
    int SkipList_level;        // ��ǰ�����в������Ľڵ����

    SkipListNode<K, V>* head;          // ָ������ͷ�ڵ�

    std::ofstream file_writer;
    std::ifstream file_reader;

    int element_count;         // ����ǰԪ�ظ���
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
�������ݣ�Ĭ�ϲ���������ظ���key��
����1��ʾԪ���Ѿ�����
����0��ʾ�ɹ�����
*/
template<typename K, typename V>
int SkipList<K, V>::insert_element(K key, V value) {
    // ��������ʱ��������Ҫ����
    Mutex.lock();

    SkipListNode<K, V>* current = head;

    // ���ȴ����������飬�����д�ŵ�����Ҫ����ǰ��ָ��forward�Ľڵ�
    // update[i]������Ҫ���µ�i��ǰ��ָ��Ľڵ�
    SkipListNode<K, V>* update[max_level + 1];
    memset(update, 0, sizeof(SkipListNode<K, V>*) * (max_level + 1));

    // �ӵ�ǰ����߲�����ʼ�������ҵ���ǰ��С�ڴ�����key�����Ԫ�أ�������Ԫ�غ󣬵�ǰ�ҵ��Ľڵ����һ���ǰ��ָ��Ӧ��ָ���²����Ԫ�أ������ҵ��Ľڵ�ŵ���Ӧ���update������
    // ��ǰ��Ľڵ��ҵ���ֻ��Ҫ �ӵ�ǰ�ڵ㿪ʼ ����Ѱ����һ��Ľڵ㼴��
    for(int i = SkipList_level; i >= 0; --i) {
        while(current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // ��������ı�������ʱ�Ѿ����˵�0�㣬��currentָ��Ľڵ������нڵ���С�ڴ�����key�����Ľڵ�
    // ���仰˵��������ڵ�Ӧ�ò嵽current���ұ�
    // ���Խ�current����һ���ڵ㣬��������ж�
    current = current->forward[0];
    if(current != NULL && current->get_key() == key) {
        std::cout << "key:  " << key << "  already exists" << std::endl;
        Mutex.unlock();
        return 1;
    }

    // ����ƶ���currentΪNULL����ô��Ҫ�ڵ�ǰλ�ò����½ڵ�
    // ����ƶ���current��key���ڴ������key����ô��Ҫ��update[0]��current֮������½ڵ�
    if(current == NULL || current->get_key() != key) {
        // ������Ҫ���һ���������
        int random_level = get_random_level();

        // ����������������� ��ǰ ���������������ôͷ�ڵ��ǰ��ָ��Ҳ��Ҫ���£����µĲ�����Χ��[��ǰ����������� + 1, �������]���������޸ĵĶ���ͷ�ڵ㣬����update������Ҳ����¼����ͷ�ڵ�
        if(random_level > SkipList_level) {
            for(int i = SkipList_level + 1; i <= random_level; ++i) {
                update[i] = head;
            }
            SkipList_level = random_level; // ���µ�ǰ������
        }

        // ����һ��������������Ľڵ�
        SkipListNode<K, V>* insert_node = create_SkipListNode(key, value, random_level);

        // ����ڵ㣨�޸�ÿһ��ǰ��ָ���ָ�򼴿ɣ�
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

// ɾ������
template <typename K, typename V>
void SkipList<K, V>::delete_element(K key) {
    // ɾ������ǰ��Ҫ����
    Mutex.lock();

    SkipListNode<K, V>* current = head;
    // ���������ʱһ����Ҫ����update����
    // update����Ĵ�Сֻ��͵�ǰ�������߲���һ�����ɣ���Ϊ��ɾ���ڵ�Ĳ���������ڵ�ǰ�������߲���
    SkipListNode<K, V>* update[SkipList_level + 1];
    memset(update, 0, sizeof(SkipListNode<K, V>*) * (SkipList_level + 1));

    for(int i = SkipList_level; i >= 0; --i) {
        while(current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // �������0��ʱ����Ҫȡcurrent�ұߵĽڵ����Key���ж�
    current = current->forward[0];
    if(current != NULL && current->get_key() == key) {
        // ͨ��update�������������ÿ���ڵ��ǰ��ָ��
        for(int i = 0; i <= current->SkipListNode_level; ++i) {
            update[i]->forward[i] = current->forward[i];
        }

        // ����Ҫ�Ե�ǰ��������������и���
        // ����ͷ�ڵ���и��£����ͷ�ڵ�ĵ� i ���ǰ��ָ��ΪNULL��˵����ǰ�����������С�� i 
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

// ��������
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


// չʾ����
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

// ��������
template<typename K, typename V>
void SkipList<K, V>::dump_file() {
    std::cout << "\n------Dump File------\n";

    file_writer.open(STORE_FILE);

    SkipListNode<K, V>* Node = head->forward[0];
    while(Node) {
        file_writer << Node->get_key() << ":" << Node->get_value() << "\n"; // �����������
        std::cout << "Key: " << Node->get_key() << " Value: " << Node->get_value() << "\n";
        Node = Node->forward[0];
    }

    file_writer.flush();
    file_writer.close();
    return;
}

// ���ļ���������
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

// �������ݹ�ģ
template<typename K, typename V>
int SkipList<K, V>::size() {
    return element_count;
}

// �Ӽ��ص����ݣ�string���л��key��value
template<typename K, typename V>
void SkipList<K, V>::get_KeyValue_from_string(const std::string& s, std::string* key, std::string* value) {
    if(!is_valid_string(s)) {
        return;
    }
    *key = s.substr(0, s.find(delimiter));
    *value = s.substr(s.find(delimiter) + 1);
}

// �жϴ��ļ���ȡ���ַ����Ƿ���Ч���Ƿ�Ϊ�գ��Ƿ��зָ�����
template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& s) {
    if(s.empty() || s.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

#endif  // KVSTORAGE_SKIPLIST_H