#include "stdafx.h"
#include "PE/PETlsTable.h"

LIBPE_NAMESPACE_BEGIN

template <class T>
UINT32
PETlsTableT<T>::GetCallbackCount()
{
    LIBPE_CHK_HR_RET(EnsureTlsCallbacksParsed(), 0);
    return (UINT32)m_vCallbackRVAs.size();
}

template <class T>
PEAddress
PETlsTableT<T>::GetCallbackRVAByIndex(_In_ UINT32 nIndex)
{
    LIBPE_CHK_HR_RET(EnsureTlsCallbacksParsed(), LIBPE_INVALID_ADDRESS);

    UINT32 nCallbackCount = GetCallbackCount();
    LIBPE_CHK(nIndex < nCallbackCount, LIBPE_INVALID_ADDRESS);

    return m_vCallbackRVAs[nIndex];
}

template <class T>
HRESULT
PETlsTableT<T>::EnsureTlsCallbacksParsed()
{
    if (m_bIsTlsCallbacksParsed) {
        return S_OK;
    }

    m_bIsTlsCallbacksParsed = true;

    LIBPE_STRICTCHK(NULL != m_pParser);
    LIBPE_CHK_HR(m_pParser->ParseTlsCallbacks(this));

    return S_OK;
}


LIBPE_FORCE_TEMPLATE_REDUCTION_CLASS(PETlsTableT);

LIBPE_NAMESPACE_END