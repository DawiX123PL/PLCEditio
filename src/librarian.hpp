#pragma once

#include <filesystem>
#include <memory>
#include "schematic_block.hpp"




class Librarian{
public:
    struct Library{
        std::filesystem::path* root;
        std::filesystem::path  path;
        std::list<Library> sub_libraries;
        std::list<std::shared_ptr<BlockData>> blocks;

        Library(){
            root = &path;
        }

        Library(std::filesystem::path _path){
            path = _path;
            root = &path;
        }

        Library(std::filesystem::path _path, std::filesystem::path* _root){
            path = _path;
            if(_root) root = _root;
            else root = &path;
        }

        void Scan(bool recursive = false){
            
            for(auto dir_entry: std::filesystem::directory_iterator(path)){

                if(!dir_entry.is_directory()) continue;

                if(dir_entry.path().extension() == ".block"){
                    BlockData block;
                    BlockData::Error err;
                    
                    err = block.Read(dir_entry.path());
                    block.SetLibraryRoot(*root);

                    if(err != BlockData::Error::OK) continue;

                    blocks.push_back(std::make_shared<BlockData>(block));
                }

                if(recursive){
                    if(dir_entry.path().extension() == ".library"){
                        sub_libraries.emplace_back(dir_entry.path(), root);
                    }
                }

            }
        }


        std::shared_ptr<BlockData> FindBlock(const std::filesystem::path& p){

            // naive solution

            for(auto& b: blocks){
                if(!b) continue;

                std::filesystem::path name = b->Name();
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

    void SetProjectPath(std::filesystem::path p){
        project_library.path = p;
    }

    void SetStdLibPath(std::filesystem::path p){
        std_library.path = p;
    }

    void Scan(){
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


    


};




























