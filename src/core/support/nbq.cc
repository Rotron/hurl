//: ----------------------------------------------------------------------------
//: Copyright (C) 2015 Verizon.  All Rights Reserved.
//: All Rights Reserved
//:
//: \file:    nbq.cc
//: \details: TODO
//: \author:  Reed P. Morrison
//: \date:    07/20/2015
//:
//:   Licensed under the Apache License, Version 2.0 (the "License");
//:   you may not use this file except in compliance with the License.
//:   You may obtain a copy of the License at
//:
//:       http://www.apache.org/licenses/LICENSE-2.0
//:
//:   Unless required by applicable law or agreed to in writing, software
//:   distributed under the License is distributed on an "AS IS" BASIS,
//:   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//:   See the License for the specific language governing permissions and
//:   limitations under the License.
//:
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: Includes
//: ----------------------------------------------------------------------------
#include "nbq.h"
#include <string.h>
#include <stdlib.h>

#include <iterator>

#include "ndebug.h"

namespace ns_hlx {

//: ----------------------------------------------------------------------------
//: Macros
//: ----------------------------------------------------------------------------
#define CHECK_FOR_NULL_AND_LEN(_buf, _len)\
        do{\
                if(!_buf) {\
                        return -1;\
                }\
                if(!_len) {\
                        return 0;\
                }\
        }while(0)\

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nbq::write(const char *a_buf, uint32_t a_len)
{
        CHECK_FOR_NULL_AND_LEN(a_buf, a_len);
        uint32_t l_left = a_len;
        uint32_t l_written = 0;
        const char *l_buf = a_buf;
        while(l_left)
        {
                if(b_write_avail() <= 0)
                {
                        int32_t l_status = b_write_add_avail();
                        if(l_status <= 0)
                        {
                                // TODO error...
                                return -1;
                        }
                }
                //NDBG_PRINT("l_left: %u\n", l_left);
                uint32_t l_write_avail = b_write_avail();
                uint32_t l_write = (l_left > l_write_avail)?l_write_avail:l_left;
                memcpy(b_write_ptr(), l_buf, l_write);
                b_write_incr(l_write);
                l_left -= l_write;
                l_buf += l_write;
                l_written += l_write;
        }
        return l_written;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nbq::read(char *a_buf, uint32_t a_len)
{
        CHECK_FOR_NULL_AND_LEN(a_buf, a_len);
        uint32_t l_read = 0;
        char *l_buf = a_buf;
        uint32_t l_total_read_avail = read_avail();
        uint32_t l_left = (a_len > l_total_read_avail)?l_total_read_avail:a_len;
        while(l_left)
        {
                uint32_t l_read_avail = b_read_avail();
                uint32_t l_read_size = (l_left > l_read_avail)?l_read_avail:l_left;

                //NDBG_PRINT("l_left:       %u\n", l_left);
                //NDBG_PRINT("l_read_avail: %u\n", l_read_avail);
                //NDBG_PRINT("l_read_size:  %u\n", l_read_size);

                memcpy(l_buf, b_read_ptr(), l_read_size);
                b_read_incr(l_read_size);
                l_left -= l_read_size;
                l_buf += l_read_size;
                l_read += l_read_size;
        }
        return l_read;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::reset_read(void)
{
        m_cur_read_block = m_q.begin();
        m_cur_block_read_ptr = NULL;
        m_total_read_avail = 0;
        if(m_q.size())
        {
                m_cur_block_read_ptr = (*m_cur_read_block)->m_data;
                m_total_read_avail = (std::distance(m_q.begin(), m_cur_write_block)+1)*m_bsize
                                     - m_cur_block_write_avail;
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::reset_write(void)
{
        if(!m_q.empty())
        {
                m_cur_write_block = m_q.begin();
                m_cur_block_write_avail = (*m_cur_write_block)->m_len;
                m_cur_block_write_ptr = (*m_cur_write_block)->m_data;
                reset_read();
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::reset(void)
{
        for(nb_list_t::iterator i_block = m_q.begin(); i_block != m_q.end(); ++i_block)
        {
                if(*i_block)
                {
                        //NDBG_PRINT("DELETING.\n");
                        delete *i_block;
                }
        }
        m_q.clear();
        m_cur_block_write_ptr = NULL;
        m_cur_block_write_avail = 0;
        m_cur_write_block = m_q.begin();
        m_cur_block_read_ptr = NULL;
        m_cur_read_block = m_q.begin();
        m_total_read_avail = 0;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::print(void)
{
        uint32_t l_total_read_avail = read_avail();
        uint32_t l_left = l_total_read_avail;
        while(l_left)
        {
                uint32_t l_read_avail = b_read_avail();
                uint32_t l_read_size = (l_left > l_read_avail)?l_read_avail:l_left;
                printf("%.*s", l_read_size, b_read_ptr());
                b_read_incr(l_read_size);
                l_left -= l_read_size;
        }
        reset_read();
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nbq::b_write_add_avail(void)
{
        nb_t *l_block = new nb_struct(m_bsize);
        m_q.push_back(l_block);
        if(m_q.size() == 1)
        {
                m_cur_read_block = m_q.begin();
                m_cur_write_block = m_q.begin();
                m_cur_block_write_ptr = (*m_cur_write_block)->m_data;
                m_cur_block_read_ptr = (*m_cur_read_block)->m_data;
        }
        else
        {
                if(!m_cur_block_write_avail &&
                  (m_cur_write_block != --m_q.end()))
                {
                        ++m_cur_write_block;
                        m_cur_block_write_ptr = (*m_cur_write_block)->m_data;
                }
        }
        m_cur_block_write_avail += m_bsize;
        return m_bsize;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::b_write_incr(uint32_t a_len)
{
        //NDBG_PRINT("%sWRITE_INCR%s[%p]: a_len: %u m_cur_block_write_avail: %d m_cur_block_write_ptr: %p m_total_read_avail: %d\n",
        //                ANSI_COLOR_BG_BLUE, ANSI_COLOR_OFF,
        //                this,
        //                a_len,
        //                (int)m_cur_block_write_avail, m_cur_block_write_ptr, (int)m_total_read_avail);

        m_cur_block_write_avail -= a_len;
        m_cur_block_write_ptr += a_len;
        m_total_read_avail += a_len;

        //NDBG_PRINT("%sWRITE_INCR%s[%p]: a_len: %u m_cur_block_write_avail: %d m_cur_block_write_ptr: %p m_total_read_avail: %d\n",
        //                ANSI_COLOR_FG_BLUE, ANSI_COLOR_OFF,
        //                this,
        //                a_len,
        //                (int)m_cur_block_write_avail, m_cur_block_write_ptr, (int)m_total_read_avail);

        if(!m_cur_block_write_avail &&
           (m_cur_write_block != --m_q.end()))
        {
                ++m_cur_write_block;
                m_cur_block_write_ptr = (*m_cur_write_block)->m_data;
                m_cur_block_write_avail = (*m_cur_write_block)->m_len;
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::b_read_incr(uint32_t a_len)
{
        uint32_t l_avail = b_read_avail();
        //NDBG_PRINT("%sREAD_INCR%s[%p]: a_len: %u l_avail: %d m_total_read_avail: %d\n", ANSI_COLOR_BG_CYAN, ANSI_COLOR_OFF, this, a_len, (int)l_avail, (int)m_total_read_avail);
        m_total_read_avail -= a_len;
        l_avail -= a_len;
        m_cur_block_read_ptr += a_len;
        //NDBG_PRINT("%sREAD_INCR%s[%p]: a_len: %u l_avail: %d m_total_read_avail: %d\n", ANSI_COLOR_FG_CYAN, ANSI_COLOR_OFF, this, a_len, (int)l_avail, (int)m_total_read_avail);
        if(!l_avail && m_total_read_avail)
        {
                ++m_cur_read_block;
                m_cur_block_read_ptr = (*m_cur_read_block)->m_data;
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
int32_t nbq::b_read_avail(void)
{
        if(m_cur_read_block == m_cur_write_block)
        {
                return m_total_read_avail;
        }
        else
        {
                return (*m_cur_read_block)->m_len - (m_cur_block_read_ptr - (*m_cur_read_block)->m_data);
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nbq::b_display_all(void)
{
        uint32_t i_block_num = 0;
        for(nb_list_t::iterator i_block = m_q.begin(); i_block != m_q.end(); ++i_block, ++i_block_num)
        {
                NDBG_OUTPUT("+------------------------------------+\n");
                NDBG_OUTPUT("| Block: %d\n", i_block_num);
                NDBG_OUTPUT("+------------------------------------+\n");
                mem_display((const uint8_t *)((*i_block)->m_data), (*i_block)->m_len);
        }
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
nbq::nbq(uint32_t a_bsize):
        m_cur_block_write_ptr(NULL),
        m_cur_block_write_avail(0),
        m_cur_write_block(),
        m_cur_block_read_ptr(NULL),
        m_cur_read_block(),
        m_total_read_avail(0),
        m_bsize(a_bsize),
        m_q()
{
        //NDBG_PRINT("%sCONSTR%s: this: %p\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF, this);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
nbq::~nbq(void)
{
        //NDBG_PRINT("%sDELETE%s: this: %p\n", ANSI_COLOR_FG_RED, ANSI_COLOR_OFF, this);
        for(nb_list_t::iterator i_block = m_q.begin(); i_block != m_q.end(); ++i_block)
        {
                if(*i_block)
                {
                        delete *i_block;
                }
        }
}

//: ----------------------------------------------------------------------------
//: Block...
//: ----------------------------------------------------------------------------

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
nb_struct::nb_struct(uint32_t a_len):
                m_data(NULL),
                m_len(0)
{
        init(a_len);
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
void nb_struct::init(uint32_t a_len)
{
        if(m_data)
        {
                free(m_data);
                m_data = NULL;
        }
        m_len = 0;
        m_data = (char *)malloc(a_len);
        //NDBG_PRINT("%sALLOC%s PTR[%p] len: %u\n", ANSI_COLOR_FG_GREEN, ANSI_COLOR_OFF, m_data, a_len);
        m_len = a_len;
}

//: ----------------------------------------------------------------------------
//: \details: TODO
//: \return:  TODO
//: \param:   TODO
//: ----------------------------------------------------------------------------
nb_struct::~nb_struct()
{
        if(m_data)
        {
                //NDBG_PRINT("%sFREE%s  PTR[%p]\n", ANSI_COLOR_FG_RED, ANSI_COLOR_OFF, m_data);
                free(m_data);
                m_data = NULL;
        }
        m_len = 0;
}

} // ns_hlx