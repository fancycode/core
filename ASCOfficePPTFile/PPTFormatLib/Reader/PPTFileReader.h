﻿/*
 * (c) Copyright Ascensio System SIA 2010-2017
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at Lubanas st. 125a-25, Riga, Latvia,
 * EU, LV-1021.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */
#pragma once

#include "../../../ASCOfficeXlsFile2/source/XlsFormat/Crypt/Decryptor.h"
#include "../../../DesktopEditor/common/Directory.h"
#include "../Records/Drawing/ArtBlip.h"

#include "PPTDocumentInfo.h"

#define CURRENT_USER_STREAM		"Current User" 

#define DOCUMENT_STREAM			"PowerPoint Document" 

#define PICTURE_STREAM			"Pictures" 
#define HEADER_STREAM			"Header" 

#define PP97_DUALSTORAGE		"PP97_DUALSTORAGE"

#define ENCRYPTION_STREAM		"EncryptedSummary" 

class CPPTFileReader
{
public: 
    CPPTFileReader(POLE::Storage *pStg, std::wstring strTemp):
		   m_pPowerPointStg(pStg),  
           m_bIsPPTFile(false),
		   m_pDocStream(NULL), 
		   m_pPictureStream(NULL),
		   m_strMemoryForder(strTemp),
		   m_oDocumentInfo()
	{ 
		m_bDualStorage = false;

		POLE::Stream *pStm = new POLE::Stream( m_pPowerPointStg, CURRENT_USER_STREAM);
				
		if ( ReadCurrentUser(pStm) ) 
		{
			m_bIsPPTFile = true; 
		}
		else
		{
			RELEASEOBJECT(pStm);
		
			std::string stream_name = std::string(PP97_DUALSTORAGE) + std::string("/") + std::string(CURRENT_USER_STREAM);
			pStm = new POLE::Stream( m_pPowerPointStg, stream_name);
			
			if (pStm == NULL)
				return;
			
			m_bDualStorage = true;
			if ( ReadCurrentUser(pStm))
			{
				m_bIsPPTFile = true; 
			}
		}
		
		RELEASEOBJECT(pStm);

        if (0 == m_strMemoryForder.length())
		{
            m_strMemoryForder = NSDirectory::GetTempPath();
		}

        m_strMemoryForder = NSDirectory::CreateDirectoryWithUniqueName(m_strMemoryForder);

	}
	~CPPTFileReader()
	{
		RELEASEOBJECT(m_pDocStream); 
		RELEASEOBJECT(m_pPictureStream); 
	
		RELEASEOBJECT(m_pPowerPointStg);
		
        NSDirectory::DeleteDirectory(m_strMemoryForder);
	}

    bool IsPowerPoint()
	{ 
		return m_bIsPPTFile;
	} 
    bool IsEncrypted()
	{
		if (m_oDocumentInfo.m_arUsers.empty()) 
			return m_oDocumentInfo.m_oCurrentUser.m_bIsEncrypt; //wps не выставляет флаг!

		return m_oDocumentInfo.m_arUsers[0]->m_bEncrypt;
	}
	CEncryptionHeader* GetEncryptionHeader()
	{
		if (m_oDocumentInfo.m_arUsers.empty()) return NULL;

		return &m_oDocumentInfo.m_arUsers[0]->m_oEncryptionHeader;
	}
	bool ReadPersists()
	{
		return m_oDocumentInfo.ReadFromStream(&m_oCurrentUser, GetDocStream());
	}
	void ReadDocument(CRYPT::ECMADecryptor *pDecryptor)
	{
		ReadPictures(pDecryptor);
		m_oDocumentInfo.LoadDocument(m_strMemoryForder);
	}

protected: 

    bool ReadCurrentUser(POLE::Stream *pStm)
	{
		if (!pStm) return false;

		SRecordHeader oHeader; 
        bool isPP = false;
		
		if( oHeader.ReadFromStream(pStm))
		{ 
			m_oCurrentUser.ReadFromStream(oHeader, pStm);
			
			isPP = (m_oCurrentUser.m_nSize == 0x00000014 && (m_oCurrentUser.m_nLenUserName <= 255)); 
		} 
 
		return isPP; 
	}

	POLE::Stream* GetDocStream()
	{ 
		if (m_pDocStream == NULL) 
		{ 
			if (!m_bIsPPTFile) 
				return NULL; 
			
			std::string stream_name;
		
			if (m_bDualStorage)	stream_name = std::string(PP97_DUALSTORAGE) + std::string("/");

			m_pDocStream = new POLE::Stream(m_pPowerPointStg, stream_name + DOCUMENT_STREAM);
		} 
		return m_pDocStream; 
 	}
	POLE::Stream* GetPictureStream()
	{ 
		if (m_pPictureStream == NULL) 
		{ 
			if (!m_bIsPPTFile) 
				return NULL; 

			std::string stream_name;

			if (m_bDualStorage)	stream_name = std::string(PP97_DUALSTORAGE) + std::string("/");

			m_pPictureStream = new POLE::Stream(m_pPowerPointStg, stream_name + PICTURE_STREAM);
		} 
		return m_pPictureStream; 
 	}

    void ReadPictures(CRYPT::ECMADecryptor *pDecryptor)
	{
		POLE::Stream* pStream = GetPictureStream();
		if (NULL == pStream) return;

		SRecordHeader oHeader;
		ULONG nRd = 0;

		while (true)
		{
            if (oHeader.ReadFromStream(pStream) == false )
			{
				break;
			}

			CRecordOfficeArtBlip art_blip;
			art_blip.m_strMemoryForder	= m_strMemoryForder;
			art_blip.m_oDocumentInfo	= &m_oDocumentInfo;

			if (pDecryptor)
			{
				POLE::Storage	*pStorageOut	= NULL;
				POLE::Stream	*pStreamTmp		= NULL;

				std::wstring sTemp = m_strMemoryForder + FILE_SEPARATOR_STR + L"~tempFile.ppt";
				
				pStorageOut	= new POLE::Storage(sTemp.c_str());		
				pStorageOut->open(true, true);

				pStreamTmp = new POLE::Stream(pStorageOut, "Tmp", true, oHeader.RecLen);

				unsigned char* data_stream = new unsigned char[oHeader.RecLen];
				pStream->read(data_stream, oHeader.RecLen);
				pDecryptor->Decrypt((char*)data_stream, oHeader.RecLen, 0);
				pStreamTmp->write(data_stream, oHeader.RecLen);
				pStreamTmp->flush();
				pStreamTmp->seek(0);
				
				art_blip.ReadFromStream(oHeader, pStreamTmp);

				delete pStream;
				delete pStorageOut;

				//NSFile::DeleteFile(sTemp);
			}
			else
			{
				art_blip.ReadFromStream(oHeader, pStream);
			}
		}
	}

 
private: 
	bool						m_bDualStorage;
	CRecordCurrentUserAtom		m_oCurrentUser; 
	POLE::Stream *				m_pDocStream; 
	POLE::Stream *				m_pPictureStream; 

	bool						m_bIsPPTFile;

public:	
	POLE::Storage*				m_pPowerPointStg; 
    std::wstring				m_strMemoryForder;

    std::vector<bool>			m_arLoadImageFlags;

	CPPTDocumentInfo			m_oDocumentInfo;
};
