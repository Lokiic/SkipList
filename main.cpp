#include <iostream>
#include "SkipList.h"

#define FILE_PATH "./store/dump_file"

int main() {
    // ��ֵ�е�key��int�ͣ�������������ͣ���Ҫ�Զ���ȽϺ���
    // ��������޸�key�����ͣ�ͬʱ��Ҫ�޸�skipList.load_file����
    SkipList<int, std::string> skipList(6);
	skipList.insert_element(1, "ѧ"); 
	skipList.insert_element(3, "�㷨"); 
	skipList.insert_element(7, "��׼"); 
	skipList.insert_element(8, "΢�Ź��ںţ���������¼"); 
	skipList.insert_element(9, "ѧϰ"); 
	skipList.insert_element(19, "�㷨����·"); 
	skipList.insert_element(19, "�Ͽ��ע����ᷢ���������"); 

    std::cout << "skipList size:" << skipList.size() << std::endl;

    // skipList.dump_file();

    // skipList.load_file();

    // skipList.find_element(9);
    // skipList.find_element(18);


    // skipList.display_list();

    // skipList.delete_element(3);
    // skipList.delete_element(7);

    // std::cout << "skipList size:" << skipList.size() << std::endl;

    // skipList.display_list();
}