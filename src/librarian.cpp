#include "librarian.hpp"

//  Librarian::Library

void Librarian::Library::Scan(bool recursive){
    std::error_code err1;
    std::filesystem::directory_iterator dir_iter(path, err1);

    if (err1) return;

    for (auto iter = std::filesystem::begin(dir_iter); iter != std::filesystem::end(dir_iter); iter++){
        const std::filesystem::directory_entry& dir_entry = *iter;

        if(!dir_entry.is_directory()) continue;

        if(dir_entry.path().extension() == ".block"){
            BlockData block;
            BlockData::Error err;
            
            err = block.Read(dir_entry.path());
            std::string name;
            try{ name = dir_entry.path().stem().string(); }
            catch(...){ name = "??????"; }
            block.SetName(name);
            block.SetNamePrefix(FullName());

            if(err != BlockData::Error::OK) continue;

            blocks.push_back(std::make_shared<BlockData>(block));
        }

        if(recursive){
            if(dir_entry.path().extension() == ".library"){
                sub_libraries.emplace_back(dir_entry.path(), this);
                sub_libraries.back().Scan(recursive);
            }
        }

    }
}



std::shared_ptr<BlockData> Librarian::Library::FindBlock(const std::filesystem::path& p){

    // naive solution

    for(auto& b: blocks){
        if(!b) continue;

        std::filesystem::path name = b->FullName();
        if(name == p) return b;
    }
    
    for(auto& lib: sub_libraries){
        auto b = lib.FindBlock(p);
        if(b) return b; 
    }

    return nullptr;

}


std::string Librarian::Library::FullName(){
    if(parent == nullptr) return name;
    else return parent->FullName() + "\\" + name;
}


Librarian::Library* Librarian::Library::getRoot(){
    if(parent == nullptr) return this;   
    else return parent->getRoot();      
}


void Librarian::Library::Clear(){
    sub_libraries.clear();
    blocks.clear();
}

