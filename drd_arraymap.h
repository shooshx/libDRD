#pragma once
// implementation of a simple mapping using a static array and O(n) search

// TKey and TKey should be some integral value (int, unsinged int, pointer etc')
template<typename TKey, typename TVal, int MaxSize>
class ArrayMap
{
public:
    ArrayMap() : m_count(0)
    {}
    bool add(TKey k, TVal v) {
        // check if it was already added
        int freeCell = -1;
        for (int i = 0; i < m_count; ++i) {
            if (m_data[i].k == k) {
                m_data[i].v = v;
                if (v == 0)
                    m_data[i].k = 0; // remove the entry
                return true;
            }
            else if (m_data[i].k == 0 && freeCell == -1) // found a free spot, remember
                freeCell = i;
        }
        if (freeCell != -1) {
            m_data[freeCell] = Entry(k, v);
            return true;
        }

        if (m_count >= MaxSize) {
            return false;
        }
        m_data[m_count++] = Entry(k, v);
        return true;
    }

    TVal get(TKey k) {
        for (int i = 0; i < m_count; ++i) {
            if (k == m_data[i].k && m_data[i].v != 0) {
                return m_data[i].v;
            }
        }
        return 0;
    }

private:
    struct Entry {
        Entry(TKey _k = 0, TVal _v = 0) : k(_k), v(_v) {}
        TKey k;
        TVal v;
    };

    Entry m_data[MaxSize];
    int m_count;
};
