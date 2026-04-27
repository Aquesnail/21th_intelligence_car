#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

#include "flag_fifo_soft.hpp"
#include "hal.hpp"

class spi_decode
{
private:
    static constexpr uint8_t k_frame_head = 0xAAu;
    static constexpr uint16_t k_sw_buffer_depth = 30u;
    static constexpr uint8_t k_max_register_count = 64u;
    static constexpr uint8_t k_max_write_addr_count = 2u;

    enum class frame_cmd : uint8_t
    {
        empty = 0u,
        write_one = 1u,
        write_two = 2u,
        read_one = 3u,
        read_two = 4u,
        streamtransfer = 5u,
        stop = 6u
    };

    enum class decode_state : uint8_t
    {
        wait_head,
        wait_mid_frame
    };

    enum class value_type : uint8_t
    {
        u8,
        i8,
        u16,
        i16,
        u32,
        i32,
        f32
    };

    struct decoded_frame
    {
        uint8_t head;
        frame_cmd cmd;
        uint8_t data1;
        uint8_t data2;
    };

    struct write_cmd
    {
        uint8_t addr;
        uint32_t value;
    };

    struct register_meta
    {
        uint8_t addr;
        void* value_ptr;
        value_type type;
        bool readable;
        bool writable;
        bool used;
    };

    template <typename T>
    struct is_supported_value_type : std::integral_constant<bool,
        std::is_same<T, uint8_t>::value ||
        std::is_same<T, int8_t>::value ||
        std::is_same<T, uint16_t>::value ||
        std::is_same<T, int16_t>::value ||
        std::is_same<T, uint32_t>::value ||
        std::is_same<T, int32_t>::value ||
        std::is_same<T, float>::value> {};

    hal_spi& scb_fifo;
    sw_flag_fifo<write_cmd, k_sw_buffer_depth> write_buffer;
    sw_flag_fifo<uint8_t, k_sw_buffer_depth> read_buffer;

    uint8_t pending_addrs[k_max_write_addr_count];
    decode_state state;
    uint8_t pending_addr_count;
    uint8_t pending_value_index;

    bool stream_transfer_on;
    uint8_t stream_addr1;
    uint8_t stream_addr2;

    register_meta reg_table[k_max_register_count];

public:
    explicit spi_decode(hal_spi& scb_fifo_)
        : scb_fifo(scb_fifo_),
          write_buffer(),
          read_buffer(),
          pending_addrs{0u, 0u},
          state(decode_state::wait_head),
          pending_addr_count(0u),
          pending_value_index(0u),
          stream_transfer_on(false),
          stream_addr1(0u),
          stream_addr2(0u),
          reg_table{}
    {
        clear_register_table();
    }

    template <typename T>
    bool register_map(uint8_t addr, T& value_ref, bool readable = true, bool writable = true)
    {
        static_assert(is_supported_value_type<T>::value, "unsupported register value type");
        return register_entry(addr,
                              static_cast<void*>(&value_ref),
                              map_type<T>(),
                              readable,
                              writable);
    }

    bool unregister_map(uint8_t addr)
    {
        for (uint8_t i = 0u; i < k_max_register_count; ++i)
        {
            if (reg_table[i].used && (reg_table[i].addr == addr))
            {
                reg_table[i].used = false;
                reg_table[i].value_ptr = nullptr;
                reg_table[i].readable = false;
                reg_table[i].writable = false;
                return true;
            }
        }

        return false;
    }

    void clear_maps()
    {
        clear_register_table();
    }

    void trig()
    {
        const bool rx_empty_at_entry = (scb_fifo.isnew() == 0u);

        while (scb_fifo.isnew() != 0u)
        {
            const uint32_t raw = scb_fifo.recive();

            if (state == decode_state::wait_head)
            {
                handle_head_frame(raw);
            }
            else
            {
                handle_mid_frame_value(raw);
            }
        }

        // 这里从 read_buffer 取命令，由元数据表直接读变量并塞入 scb_fifo。
        drain_read_buffer_to_scb();
        // 这里从 write_buffer 取命令，由元数据表直接写变量并校验可写性。
        drain_write_buffer_to_registers();

        if (stream_transfer_on && rx_empty_at_entry && (state == decode_state::wait_head))
        {
            emit_stream_payload();
        }
    }

private:
    template <typename T>
    static value_type map_type()
    {
        if (std::is_same<T, uint8_t>::value)
        {
            return value_type::u8;
        }
        if (std::is_same<T, int8_t>::value)
        {
            return value_type::i8;
        }
        if (std::is_same<T, uint16_t>::value)
        {
            return value_type::u16;
        }
        if (std::is_same<T, int16_t>::value)
        {
            return value_type::i16;
        }
        if (std::is_same<T, uint32_t>::value)
        {
            return value_type::u32;
        }
        if (std::is_same<T, int32_t>::value)
        {
            return value_type::i32;
        }
        return value_type::f32;
    }

    static decoded_frame decode_raw_frame(uint32_t raw)
    {
        decoded_frame frame{};
        frame.head = static_cast<uint8_t>(raw & 0xFFu);
        frame.cmd = static_cast<frame_cmd>((raw >> 8) & 0xFFu);
        frame.data1 = static_cast<uint8_t>((raw >> 16) & 0xFFu);
        frame.data2 = static_cast<uint8_t>((raw >> 24) & 0xFFu);
        return frame;
    }

    void handle_head_frame(uint32_t raw)
    {
        const decoded_frame frame = decode_raw_frame(raw);
        if (frame.head != k_frame_head)
        {
            return;
        }

        switch (frame.cmd)
        {
            case frame_cmd::read_one:
            {
                read_buffer.write(frame.data1);
                break;
            }
            case frame_cmd::read_two:
            {
                read_buffer.write(frame.data1);
                read_buffer.write(frame.data2);
                break;
            }
            case frame_cmd::write_one:
            {
                pending_addrs[0] = frame.data1;
                pending_addr_count = 1u;
                pending_value_index = 0u;
                state = decode_state::wait_mid_frame;
                break;
            }
            case frame_cmd::write_two:
            {
                pending_addrs[0] = frame.data1;
                pending_addrs[1] = frame.data2;
                pending_addr_count = 2u;
                pending_value_index = 0u;
                state = decode_state::wait_mid_frame;
                break;
            }
            case frame_cmd::streamtransfer:
            {
                stream_transfer_on = true;
                stream_addr1 = frame.data1;
                stream_addr2 = frame.data2;
                break;
            }
            case frame_cmd::stop:
            {
                stream_transfer_on = false;
                break;
            }
            case frame_cmd::empty:
            default:
            {
                break;
            }
        }
    }

    void handle_mid_frame_value(uint32_t raw)
    {
        if (pending_value_index < pending_addr_count)
        {
            write_cmd cmd{};
            cmd.addr = pending_addrs[pending_value_index];
            cmd.value = raw;
            write_buffer.write(cmd);
            ++pending_value_index;
        }

        if (pending_value_index >= pending_addr_count)
        {
            clear_pending_write_context();
        }
    }

    void drain_read_buffer_to_scb()
    {
        while (read_buffer.isNew() != 0u)
        {
            const uint8_t addr = read_buffer.read();
            const register_meta* meta = find_entry(addr);

            if ((meta == nullptr) || (!meta->readable))
            {
                continue;
            }

            scb_fifo.send(read_register_as_u32(*meta));
        }
    }

    void drain_write_buffer_to_registers()
    {
        while (write_buffer.isNew() != 0u)
        {
            const write_cmd cmd = write_buffer.read();
            register_meta* meta = find_entry(cmd.addr);

            if ((meta == nullptr) || (!meta->writable))
            {
                continue;
            }

            write_u32_to_register(*meta, cmd.value);
        }
    }

    void emit_stream_payload()
    {
        emit_read_value_by_addr(stream_addr1);
        emit_read_value_by_addr(stream_addr2);
    }

    void emit_read_value_by_addr(uint8_t addr)
    {
        const register_meta* meta = find_entry(addr);
        if ((meta == nullptr) || (!meta->readable))
        {
            return;
        }

        scb_fifo.send(read_register_as_u32(*meta));
    }

    bool register_entry(uint8_t addr, void* value_ptr, value_type type, bool readable, bool writable)
    {
        if (value_ptr == nullptr)
        {
            return false;
        }

        register_meta* old = find_entry(addr);
        if (old != nullptr)
        {
            old->value_ptr = value_ptr;
            old->type = type;
            old->readable = readable;
            old->writable = writable;
            old->used = true;
            return true;
        }

        for (uint8_t i = 0u; i < k_max_register_count; ++i)
        {
            if (!reg_table[i].used)
            {
                reg_table[i].addr = addr;
                reg_table[i].value_ptr = value_ptr;
                reg_table[i].type = type;
                reg_table[i].readable = readable;
                reg_table[i].writable = writable;
                reg_table[i].used = true;
                return true;
            }
        }

        return false;
    }

    register_meta* find_entry(uint8_t addr)
    {
        for (uint8_t i = 0u; i < k_max_register_count; ++i)
        {
            if (reg_table[i].used && (reg_table[i].addr == addr))
            {
                return &reg_table[i];
            }
        }

        return nullptr;
    }

    const register_meta* find_entry(uint8_t addr) const
    {
        for (uint8_t i = 0u; i < k_max_register_count; ++i)
        {
            if (reg_table[i].used && (reg_table[i].addr == addr))
            {
                return &reg_table[i];
            }
        }

        return nullptr;
    }

    static uint32_t read_register_as_u32(const register_meta& meta)
    {
        switch (meta.type)
        {
            case value_type::u8:
            {
                return static_cast<uint32_t>(*static_cast<const uint8_t*>(meta.value_ptr));
            }
            case value_type::i8:
            {
                return static_cast<uint32_t>(static_cast<uint8_t>(*static_cast<const int8_t*>(meta.value_ptr)));
            }
            case value_type::u16:
            {
                return static_cast<uint32_t>(*static_cast<const uint16_t*>(meta.value_ptr));
            }
            case value_type::i16:
            {
                return static_cast<uint32_t>(static_cast<uint16_t>(*static_cast<const int16_t*>(meta.value_ptr)));
            }
            case value_type::u32:
            {
                return *static_cast<const uint32_t*>(meta.value_ptr);
            }
            case value_type::i32:
            {
                return static_cast<uint32_t>(*static_cast<const int32_t*>(meta.value_ptr));
            }
            case value_type::f32:
            {
                uint32_t raw = 0u;
                const float value = *static_cast<const float*>(meta.value_ptr);
                std::memcpy(&raw, &value, sizeof(raw));
                return raw;
            }
            default:
            {
                return 0u;
            }
        }
    }

    static void write_u32_to_register(register_meta& meta, uint32_t raw)
    {
        switch (meta.type)
        {
            case value_type::u8:
            {
                *static_cast<uint8_t*>(meta.value_ptr) = static_cast<uint8_t>(raw);
                break;
            }
            case value_type::i8:
            {
                *static_cast<int8_t*>(meta.value_ptr) = static_cast<int8_t>(raw);
                break;
            }
            case value_type::u16:
            {
                *static_cast<uint16_t*>(meta.value_ptr) = static_cast<uint16_t>(raw);
                break;
            }
            case value_type::i16:
            {
                *static_cast<int16_t*>(meta.value_ptr) = static_cast<int16_t>(raw);
                break;
            }
            case value_type::u32:
            {
                *static_cast<uint32_t*>(meta.value_ptr) = raw;
                break;
            }
            case value_type::i32:
            {
                *static_cast<int32_t*>(meta.value_ptr) = static_cast<int32_t>(raw);
                break;
            }
            case value_type::f32:
            {
                float value = 0.0f;
                std::memcpy(&value, &raw, sizeof(value));
                *static_cast<float*>(meta.value_ptr) = value;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    void clear_pending_write_context()
    {
        pending_addrs[0] = 0u;
        pending_addrs[1] = 0u;
        pending_addr_count = 0u;
        pending_value_index = 0u;
        state = decode_state::wait_head;
    }

    void clear_register_table()
    {
        for (uint8_t i = 0u; i < k_max_register_count; ++i)
        {
            reg_table[i].addr = 0u;
            reg_table[i].value_ptr = nullptr;
            reg_table[i].type = value_type::u32;
            reg_table[i].readable = false;
            reg_table[i].writable = false;
            reg_table[i].used = false;
        }
    }
};
