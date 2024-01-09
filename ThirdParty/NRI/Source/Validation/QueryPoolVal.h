// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct QueryPoolVal : public DeviceObjectVal<QueryPool> {
    QueryPoolVal(DeviceVal& device, QueryPool* queryPool, QueryType queryType, uint32_t queryNum);

    bool SetQueryState(uint32_t offset, bool state);
    void ResetQueries(uint32_t offset, uint32_t number);

    inline uint32_t GetQueryNum() const {
        return m_QueryNum;
    }

    inline QueryType GetQueryType() const {
        return m_QueryType;
    }

    inline bool IsImported() const {
        return m_QueryNum == 0;
    }

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    uint32_t GetQuerySize() const;

  private:
    Vector<uint64_t> m_DeviceState;
    uint32_t m_QueryNum;
    QueryType m_QueryType;
};

} // namespace nri
