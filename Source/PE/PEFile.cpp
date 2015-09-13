#include "stdafx.h"
#include "PE/PEFile.h"

LIBPE_NAMESPACE_BEGIN

template <class T>
HRESULT
PEFileT<T>::Init(_In_ PEParserT<T> *pParser)
{
    m_pParser = pParser;

    HRESULT hr = pParser->ParseBasicInfo(&m_pDosHeader, &m_pNtHeaders, &m_vSectionHeaders, &m_pOverlay);
    if(FAILED(hr) || NULL == m_pDosHeader || NULL == m_pNtHeaders) {
        return E_FAIL;
    }

    m_pNtHeaders->GetFileHeader(&m_pFileHeader);
    m_pNtHeaders->GetOptionalHeader(&m_pOptionalHeader);

    return S_OK;
}

template <class T>
HRESULT
PEFileT<T>::Create(_In_ DataLoader *pLoader, _Outptr_ IPEFile **ppFile)
{
    LIBPE_CHK(NULL != pLoader && NULL != ppFile, E_POINTER);

    *ppFile = NULL;

    LibPEPtr<PEFileT<T>> pInnerFile = new PEFileT<T>();
    LibPEPtr<PEParserT<T>> pParser = PEParserT<T>::Create(pLoader->GetType());
    LIBPE_CHK(NULL != pInnerFile && NULL != pParser, E_OUTOFMEMORY);

    pParser->SetPEFile(pInnerFile);
    pParser->SetDataLoader(pLoader);

    if(FAILED(pInnerFile->Init(pParser))) {
        return E_FAIL;
    }

    *ppFile = pInnerFile.Detach();

    return S_OK;
}

template <class T>
PERawDosHeader *
PEFileT<T>::GetRawDosHeader()
{
    LIBPE_CHK(NULL != m_pDosHeader, NULL);
    return (PERawDosHeader *)m_pDosHeader->GetRawMemory();
}

template <class T>
void *
PEFileT<T>::GetRawNtHeaders()
{
    LIBPE_CHK(NULL != m_pNtHeaders, NULL);
    return m_pNtHeaders->GetRawMemory();
}

template <class T>
PERawNtHeaders32 *
PEFileT<T>::GetRawNtHeaders32()
{
    return (PERawNtHeaders32 *)(Is32Bit() ? GetRawNtHeaders() : NULL);
}

template <class T>
PERawNtHeaders64 *
PEFileT<T>::GetRawNtHeaders64()
{
    LIBPE_CHK(NULL != m_pNtHeaders, NULL);
    return (PERawNtHeaders64 *)(Is32Bit() ? NULL : GetRawNtHeaders());
}

template <class T>
PERawFileHeader *
PEFileT<T>::GetRawFileHeader()
{
    LIBPE_CHK(NULL != m_pFileHeader, NULL);
    return (PERawFileHeader *)m_pFileHeader->GetRawMemory();
}

template <class T>
void *
PEFileT<T>::GetRawOptionalHeader()
{
    LIBPE_CHK(NULL != m_pOptionalHeader, NULL);
    return m_pOptionalHeader->GetRawMemory();
}

template <class T>
PERawOptionalHeader32 *
PEFileT<T>::GetRawOptionalHeader32()
{
    return (PERawOptionalHeader32 *)(Is32Bit() ? GetRawOptionalHeader() : NULL);
}

template <class T>
PERawOptionalHeader64 *
PEFileT<T>::GetRawOptionalHeader64()
{
    return (PERawOptionalHeader64 *)(Is32Bit() ? GetRawOptionalHeader() : NULL);
}

template <class T>
HRESULT
PEFileT<T>::GetDosHeader(_Outptr_ IPEDosHeader **ppDosHeader)
{
    return m_pDosHeader.CopyTo(ppDosHeader);
}

template <class T>
HRESULT
PEFileT<T>::GetNtHeaders(_Outptr_ IPENtHeaders **ppNtHeaders)
{
    return m_pNtHeaders.CopyTo(ppNtHeaders);
}

template <class T>
HRESULT
PEFileT<T>::GetFileHeader(_Outptr_ IPEFileHeader **ppFileHeader)
{
    LIBPE_CHK(NULL != m_pNtHeaders, NULL);
    return m_pNtHeaders->GetFileHeader(ppFileHeader);
}

template <class T>
HRESULT
PEFileT<T>::GetOptionalHeader(_Outptr_ IPEOptionalHeader **ppOptionalHeader)
{
    LIBPE_CHK(NULL != m_pNtHeaders, NULL);
    return m_pNtHeaders->GetOptionalHeader(ppOptionalHeader);
}

template <class T>
BOOL
PEFileT<T>::IsDosFile()
{
    LIBPE_CHK(NULL != m_pDosHeader, false);
    return (NULL == m_pDosHeader->GetFieldLfanew());
}

template <class T>
BOOL
PEFileT<T>::Is32Bit()
{
    return PETrait<T>::PointerSize == 4;
}

template <class T>
PEAddress
PEFileT<T>::GetImageBase()
{
    LIBPE_CHK(NULL != m_pOptionalHeader, NULL);
    return m_pOptionalHeader->GetFieldImageBase();
}

template <class T>
UINT32 
PEFileT<T>::GetImageSize()
{
    LIBPE_CHK(NULL != m_pOptionalHeader, 0);
    return m_pOptionalHeader->GetFieldSizeOfImage();
}

template <class T>
UINT32 
PEFileT<T>::GetEntryPoint()
{
    LIBPE_CHK(NULL != m_pOptionalHeader, 0);
    return m_pOptionalHeader->GetFieldAddressOfEntryPoint();
}

template <class T>
UINT32
PEFileT<T>::GetSectionCount()
{
    return (UINT32)m_vSectionHeaders.size();
}

template <class T>
HRESULT
PEFileT<T>::GetSectionHeader(_In_ UINT32 nIndex, _Outptr_ IPESectionHeader **ppSectionHeader)
{
    LIBPE_CHK(NULL != ppSectionHeader, E_POINTER);
    LIBPE_CHK(nIndex < GetSectionCount(), E_INVALIDARG);
    return m_vSectionHeaders[nIndex].CopyTo(ppSectionHeader);
}

template <class T>
HRESULT
PEFileT<T>::GetSection(_In_ UINT32 nIndex, _Outptr_ IPESection **ppSection)
{
    LIBPE_CHK(NULL != ppSection, E_POINTER);
    LIBPE_CHK(nIndex < GetSectionCount(), E_INVALIDARG);
    LIBPE_CHK(NULL != m_vSectionHeaders[nIndex], E_FAIL);
    return m_vSectionHeaders[nIndex]->GetSection(ppSection);
}

template <class T>
HRESULT
PEFileT<T>::GetSectionByRVA(_In_ PEAddress nRVA, _Outptr_ IPESection **ppSection)
{
    LIBPE_CHK(LIBPE_INVALID_ADDRESS != nRVA, E_INVALIDARG);
    LIBPE_CHK(NULL != ppSection, E_POINTER);

    UINT32 nSectionCount = GetSectionCount();
    for(UINT32 nSectionIndex = 0; nSectionIndex < nSectionCount; ++nSectionIndex) {
        LibPEPtr<IPESection> pSection;
        if(SUCCEEDED(GetSection(nSectionIndex, &pSection)) && NULL != pSection) {
            if(pSection->GetRVA() <= nRVA && nRVA <= pSection->GetRVA() + pSection->GetSizeInMemory()) {
                *ppSection = pSection.Detach();
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

template <class T>
HRESULT
PEFileT<T>::GetSectionByVA(_In_ PEAddress nVA, _Outptr_ IPESection **ppSection)
{
    return GetSectionByRVA(GetRVAFromVA(nVA), ppSection);
}

template <class T>
HRESULT
PEFileT<T>::GetSectionByFOA(_In_ PEAddress nFOA, _Outptr_ IPESection **ppSection)
{
    LIBPE_CHK(LIBPE_INVALID_ADDRESS != nFOA, E_INVALIDARG);
    LIBPE_CHK(NULL != ppSection, E_POINTER);

    UINT32 nSectionCount = GetSectionCount();
    for(UINT32 nSectionIndex = 0; nSectionIndex < nSectionCount; ++nSectionIndex) {
        LibPEPtr<IPESection> pSection;
        if(SUCCEEDED(GetSection(nSectionIndex, &pSection)) && NULL != pSection) {
            if(pSection->GetFOA() <= nFOA && nFOA <= pSection->GetFOA() + pSection->GetSizeInMemory()) {
                *ppSection = pSection.Detach();
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

template <class T>
HRESULT
PEFileT<T>::GetOverlay(_Outptr_ IPEOverlay **ppOverlay)
{
    LIBPE_CHK(NULL != ppOverlay, E_POINTER);
    return m_pOverlay.CopyTo(ppOverlay);
}

template <class T>
PEAddress
PEFileT<T>::GetRVAFromVA(_In_ PEAddress nVA)
{
    LIBPE_STRICTCHK(NULL != m_pParser);
    return m_pParser->GetRVAFromVA(nVA);
}

template <class T>
PEAddress
PEFileT<T>::GetVAFromRVA(_In_ PEAddress nRVA)
{
    LIBPE_STRICTCHK(NULL != m_pParser);
    return m_pParser->GetVAFromRVA(nRVA);
}

template <class T>
PEAddress
PEFileT<T>::GetRVAFromFOA(_In_ PEAddress nFOA)
{
    LIBPE_STRICTCHK(NULL != m_pParser);
    return m_pParser->GetRVAFromFOA(nFOA);
}

template <class T>
PEAddress
PEFileT<T>::GetFOAFromRVA(_In_ PEAddress nRVA)
{
    LIBPE_STRICTCHK(NULL != m_pParser);
    return m_pParser->GetFOAFromRVA(nRVA);
}

template <class T>
PEAddress
PEFileT<T>::GetVAFromFOA(_In_ PEAddress nFOA)
{
    LIBPE_STRICTCHK(NULL != m_pParser);
    return m_pParser->GetVAFromFOA(nFOA);
}

template <class T>
PEAddress
PEFileT<T>::GetFOAFromVA(_In_ PEAddress nVA)
{
    LIBPE_STRICTCHK(NULL != m_pParser);
    return m_pParser->GetFOAFromVA(nVA);
}

template <class T>
HRESULT
PEFileT<T>::GetExportTable(_Outptr_ IPEExportTable **ppExportTable)
{
    return ParsePETable(&PEParserT<T>::ParseExportTable, TPF_IsExportTableParsed, m_pExportTable, ppExportTable);
}

template <class T>
HRESULT
PEFileT<T>::GetImportTable(_Outptr_ IPEImportTable **ppImportTable)
{
    return ParsePETable(&PEParserT<T>::ParseImportTable, TPF_IsImportTableParsed, m_pImportTable, ppImportTable);
}

template <class T>
HRESULT
PEFileT<T>::GetResourceTable(_Outptr_ IPEResourceTable **ppResourceTable)
{
    return ParsePETable(&PEParserT<T>::ParseResourceTable, TPF_IsResourceTableParsed, m_pResourceTable, ppResourceTable);
}

template <class T>
HRESULT
PEFileT<T>::GetExceptionTable(_Outptr_ IPEExceptionTable **ppExceptionTable)
{
    return ParsePETable(&PEParserT<T>::ParseExceptionTable, TPF_IsExceptionTableParsed, m_pExceptionTable, ppExceptionTable);
}

template <class T>
HRESULT
PEFileT<T>::GetCertificateTable(_Outptr_ IPECertificateTable **ppCertificateTable)
{
    return ParsePETable(&PEParserT<T>::ParseCertificateTable, TPF_IsCertificateTableParsed, m_pCertificateTable, ppCertificateTable);
}

template <class T>
HRESULT
PEFileT<T>::GetRelocationTable(_Outptr_ IPERelocationTable **ppRelocationTable)
{
    return ParsePETable(&PEParserT<T>::ParseRelocationTable, TPF_IsRelocationTableParsed, m_pRelocationTable, ppRelocationTable);
}

template <class T>
HRESULT
PEFileT<T>::GetDebugInfoTable(_Outptr_ IPEDebugInfoTable **ppDebugInfoTable)
{
    return ParsePETable(&PEParserT<T>::ParseDebugInfoTable, TPF_IsDebugInfoTableParsed, m_pDebugInfoTable, ppDebugInfoTable);
}

template <class T>
HRESULT
PEFileT<T>::GetArchitectureDataTable(_Outptr_ IPEArchitectureDataTable **ppArchitectureDataTable)
{
    LIBPE_CHK(NULL != ppArchitectureDataTable, E_POINTER);

    // According to PE and COFF specification 8.3, the architecture data table is reserved and must be 0.
    // If there is any data in this table, we need to check what is the data exactly and write the code then.
    // Link: http://msdn.microsoft.com/en-us/library/windows/hardware/gg463119.aspx
    *ppArchitectureDataTable = NULL;

    return E_NOT_SET;
}

template <class T>
HRESULT 
PEFileT<T>::GetGlobalPointerTable(_Outptr_ IPEGlobalPointerTable **ppGlobalPointerTable)
{
    return ParsePETable(&PEParserT<T>::ParseGlobalPointerTable, TPF_IsGlobalPointerTableParsed, m_pGlobalPointerTable, ppGlobalPointerTable);
}

template <class T>
HRESULT
PEFileT<T>::GetTlsTable(_Outptr_ IPETlsTable **ppTlsTable)
{
    return ParsePETable(&PEParserT<T>::ParseTlsTable, TPF_IsTlsTableParsed, m_pTlsTable, ppTlsTable);
}

template <class T>
HRESULT
PEFileT<T>::GetLoadConfigTable(_Outptr_ IPELoadConfigTable **ppLoadConfigTable)
{
    return ParsePETable(&PEParserT<T>::ParseLoadConfigTable, TPF_IsLoadConfigTableParsed, m_pLoadConfigTable, ppLoadConfigTable);
}

template <class T>
HRESULT
PEFileT<T>::GetBoundImportTable(_Outptr_ IPEBoundImportTable **ppBoundImportTable)
{
    return ParsePETable(&PEParserT<T>::ParseBoundImportTable, TPF_IsBoundImportTableParsed, m_pBoundImportTable, ppBoundImportTable);
}

template <class T>
HRESULT
PEFileT<T>::GetImportAddressTable(_Outptr_ IPEImportAddressTable **ppImportAddressTable)
{
    return ParsePETable(&PEParserT<T>::ParseImportAddressTable, TPF_IsImportAddressTableParsed, m_pImportAddressTable, ppImportAddressTable);
}

template <class T>
HRESULT
PEFileT<T>::GetDelayImportTable(_Outptr_ IPEDelayImportTable **ppDelayImportTable)
{
    return ParsePETable(&PEParserT<T>::ParseDelayImportTable, TPF_IsDelayImportTableParsed, m_pDelayImportTable, ppDelayImportTable);
}

template <class T>
HRESULT
PEFileT<T>::GetClrTable(_Outptr_ IPEClrTable **ppClrTable)
{
    return ParsePETable(&PEParserT<T>::ParseClrTable, TPF_IsClrTableParsed, m_pClrTable, ppClrTable);
}

LIBPE_FORCE_TEMPLATE_REDUCTION_CLASS(PEFileT);
LIBPE_FORCE_TEMPLATE_REDUCTION_CLASS_FUNCTION(PEFileT, Create);

LIBPE_NAMESPACE_END