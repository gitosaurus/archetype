//
//  FileStorage.h
//  archetype
//
//  Created by Derek Jones on 9/2/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__FileStorage__
#define __archetype__FileStorage__

#include <filesystem>
#include <iostream>
#include <fstream>
#include <span>
#include <string>

#include "Serialization.hh"

namespace archetype {
    class InFileStorage : public Storage {
    public:
        InFileStorage(std::string filename);
        InFileStorage(const InFileStorage&) = delete;
        InFileStorage& operator=(const InFileStorage&) = delete;

        [[nodiscard]] bool ok() const;
        virtual ~InFileStorage() { }
        virtual int remaining() const override;
        virtual int read(std::span<Byte> buf) override;
        virtual void write(std::span<const Byte> buf) override;
    private:
        std::ifstream stream_;
        int remaining_;
    };

    class OutFileStorage : public Storage {
    public:
        OutFileStorage(std::string filename);
        OutFileStorage(const OutFileStorage&) = delete;
        OutFileStorage& operator=(const OutFileStorage&) = delete;

        // True while the file is open and every write so far has succeeded.
        // Worth calling *after* writing as well as before: a disk that fills up
        // mid-save shows up here and nowhere else.
        [[nodiscard]] bool ok() const;

        // Flush and close, reporting the final state of the stream.  The
        // destructor would otherwise close the file and discard any error.
        [[nodiscard]] bool commit();

        virtual ~OutFileStorage() { }
        virtual int remaining() const override;
        virtual int read(std::span<Byte> buf) override;
        virtual void write(std::span<const Byte> buf) override;
    private:
        std::ofstream stream_;
        bool failed_;
    };

    // Replace the contents of path with bytes, without ever leaving path
    // partially written: the bytes go to a sibling temporary file, which is
    // then renamed over path.  With keep_backup, the previous contents are
    // first rotated to path + ".bak".
    //
    // Crash-safe against process death, not against power loss: there is no
    // fsync, so the rename may be durable before the data it points at.
    //
    // Returns false and fills error_out on failure, in which case path is left
    // as it was.
    bool writeBytesAtomically(const std::filesystem::path& path,
                              std::span<const Storage::Byte> bytes,
                              bool keep_backup,
                              std::string& error_out);

    // Serialize the current Universe and install it at path via
    // writeBytesAtomically.  Serializing to memory first means a failure part
    // way through never reaches the disk at all.
    bool writeUniverseAtomically(const std::filesystem::path& path,
                                 bool keep_backup,
                                 std::string& error_out);
}

#endif /* defined(__archetype__FileStorage__) */
