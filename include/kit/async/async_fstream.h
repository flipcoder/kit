#ifndef FILE_H_VSWBGI51
#define FILE_H_VSWBGI51

#include <fstream>
#include <string>
#include <future>
#include <atomic>
#include "async.h"
#include "../kit.h"

class async_fstream
{
    public:

        async_fstream(Multiplexer::Circuit* circuit = &MX[0]):
            m_pCircuit(circuit)
        {}
        //async_fstream(
        //    std::string fn, Multiplexer::Circuit* circuit = &MX[0]
        //):
        //    m_pCircuit(circuit),
        //    m_Filename(fn)
        //{
        //    m_pCircuit->task<void>([this, fn]{
        //        m_File.open(fn);
        //        m_Filename = fn;
        //    });
        //}
        ~async_fstream() {
            close().get();
        }

        async_fstream(const async_fstream&) = delete;
        async_fstream(async_fstream&&) = default;
        async_fstream& operator=(const async_fstream&) = default;
        async_fstream& operator=(async_fstream&&) = default;

        std::future<void> open(
            std::ios_base::openmode mode = std::ios_base::in|std::ios_base::out
        ){
            return m_pCircuit->task<void>([this, mode]{
                m_File.open(m_Filename, mode);
            });
        }
        std::future<void> open(
            std::string fn,
            std::ios_base::openmode mode = std::ios_base::in|std::ios_base::out
        ){
            return m_pCircuit->task<void>([this, fn, mode]{
                _close();
                m_File.open(fn, mode);
                m_Filename = fn;
            });
        }

        std::future<bool> is_open() {
            return m_pCircuit->task<bool>([this]{
                return m_File.is_open();
            });
        }
        std::future<void> close() {
            return m_pCircuit->task<void>([this]{
                _close();
            });
        }
        std::future<std::string> filename() const {
            return m_pCircuit->task<std::string>(
                [this]{return m_Filename;}
            );
        };
        std::future<std::string> buffer() const {
            return m_pCircuit->task<std::string>([this]{
                _cache();
                return m_Buffer;
            });
        }

        template<class T>
        std::future<T> with(std::function<T(std::fstream& f)> func) {
            return m_pCircuit->task<T>([this,func]{ return func(m_File); });
        }
        template<class T>
        std::future<T> with(std::function<T(const std::string&)> func) {
            return m_pCircuit->task<T>([this,func]{
                _cache();
                return func(m_Buffer);
            });
        }

        std::future<void> invalidate() {
            return m_pCircuit->task<void>(
                [this]{_invalidate();}
            );
        }

        std::future<void> recache() {
            return m_pCircuit->task<void>(
                [this]{_invalidate();_cache();}
            );
        }
        std::future<void> cache() const {
            return m_pCircuit->task<void>(
                [this]{_cache();}
            );
        }

    private:

        void _close() {
            m_Filename = std::string();
            _invalidate();
            //if(m_File.is_open()
            m_File.close();
        }

        void _invalidate() {
            m_Buffer.clear();
        }
        void _cache() const {
            if(m_Buffer.empty()){
                m_File.seekg(0, std::ios::end);
                m_Buffer.reserve(m_File.tellg());
                m_File.seekg(0, std::ios::beg);
                m_Buffer.assign(
                    (std::istreambuf_iterator<char>(m_File)),
                    std::istreambuf_iterator<char>()
                );
            }            
        }

        Multiplexer::Circuit* const m_pCircuit;
        mutable std::fstream m_File;
        std::string m_Filename;

        mutable std::string m_Buffer;
};

#endif

