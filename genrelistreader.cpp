/***************************************************************************
                          genrelistreader.cpp  -  description
                             -------------------
    begin                : Mon Mar 4 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "genrelistreader.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreListReader::GenreListReader(GenreList *genreList)
{
  list = genreList;
}

GenreListReader::~GenreListReader()
{
}

bool GenreListReader::startElement(const QString&, const QString&, const QString& element, const QXmlAttributes& attributes)
{
  if(element.lower() == "genre") {
    inGenreTag = true;
    if(attributes.index("id3v1") != -1)
      id3v1 = attributes.value("id3v1").toInt();
    else 
      id3v1 = 255;
  }
  else {
    id3v1 = 255;
  }
  return(true);
};

bool GenreListReader::endElement(const QString&, const QString&, const QString& element)
{
  if(element.lower() == "genre")
    inGenreTag = false;
  return(true);
};
  
bool GenreListReader::characters(const QString& content)
{
  if(inGenreTag) {
    list->append(Genre(content, id3v1));
  }
  return(true);
};
