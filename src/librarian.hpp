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

        std::string FullName();
        Library* getRoot();
        void Clear();
        void Scan(bool recursive = false);
        std::shared_ptr<BlockData> FindBlock(const std::filesystem::path& p);

    };


private:

    Library* project_library;
    Library* std_library;
    Library global_library;


public:

    Librarian(){
        // global_library.sub_libraries[0] - LOCAL LIB
        // global_library.sub_libraries[1] - STD LIB

        project_library = &global_library.sub_libraries.emplace_back("", &global_library);
        std_library     = &global_library.sub_libraries.emplace_back("", &global_library);

        project_library->name = "LOCAL";
        std_library->name = "STD";

    }

    void SetProjectPath(std::filesystem::path p){
        project_library->path = p;
    }

    void SetStdLibPath(std::filesystem::path p){
        std_library->path = p;
    }

    // Library& GetProjectLib(){
    //     return *project_library;
    // }

    // Library& GetStdLib(){
    //     return *std_library;
    // }


    Library& GetLib(){
        return global_library;
    }


    void Scan(){
        project_library->Clear();
        std_library->Clear();
        project_library->Scan(true);
        std_library->Scan(true);
    }

    std::shared_ptr<BlockData> FindBlock(const std::filesystem::path& p){
        std::shared_ptr<BlockData> b;

        b = project_library->FindBlock(p);
        if(b) return b;

        b = std_library->FindBlock(p);
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
        project_library->Clear();
        project_library->Scan(true);
    }


    


};




























