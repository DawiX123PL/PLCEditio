#pragma once

#include <filesystem>
#include <memory>
#include "schematic_block.hpp"




class Librarian{
public:

    class Library{
        Library* parent;
    public:
        std::string name;
        std::filesystem::path path;
        std::list<Library> sub_libraries;
        std::list<std::shared_ptr<BlockData>> blocks;



        Library(): parent(nullptr){
            name = "";
            path = "";
        }

        Library(std::filesystem::path _path): parent(nullptr){
            path = _path;
            try{
                name = path.stem().string();
            }catch(...){
                name = "???";
            }
        }

        Library(std::filesystem::path _path, Library* _parent): parent(_parent){
            path = _path;
            try{
                name = path.stem().string();
            }catch(...){
                name = "???";
            }
        }

        std::string FullName(){
            if(parent == nullptr) return name;
            else return parent->FullName() + "\\" + name;
        }

        Library* getRoot(){
            if(parent == nullptr) return this;   
            else return parent->getRoot();      
        }



        void Scan(bool recursive = false){
            
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
                    block.SetLibraryRoot(getRoot()->path);

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

        void Clear(){
            sub_libraries.clear();
            blocks.clear();
        }


        std::shared_ptr<BlockData> FindBlock(const std::filesystem::path& p){

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


    };


private:

    Library project_library;
    Library std_library;


public:

    Librarian(){
        project_library.Clear();
        std_library.Clear();
        project_library.name = "Local";
        std_library.name = "STD";    
    }

    void SetProjectPath(std::filesystem::path p){
        project_library.path = p;
    }

    void SetStdLibPath(std::filesystem::path p){
        std_library.path = p;
    }

    Library& GetProjectLib(){
        return project_library;
    }

    Library& GetStdLib(){
        return std_library;
    }


    void Scan(){
        project_library.Clear();
        std_library.Clear();
        project_library.Scan(true);
        std_library.Scan(true);
    }

    std::shared_ptr<BlockData> FindBlock(const std::filesystem::path& p){
        std::shared_ptr<BlockData> b;

        b = project_library.FindBlock(p);
        if(b) return b;

        b = std_library.FindBlock(p);
        if(b) return b;

        return nullptr;
    }

    // TODO: Implement this
    void CopyLocalLibTo(std::filesystem::path p){

    }

    void AddBlock(BlockData block){
        // naive solution
        block.SetDemoBlockData();
        BlockData::Error err = block.Save();
        if (err != BlockData::Error::OK) return;
        
        
        Scan();
    }


    void ScanProject(){
        project_library.Clear();
        project_library.Scan(true);
    }


    


};




























