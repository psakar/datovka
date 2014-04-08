
#*    dslib - Python library for Datove schranky
#*    Copyright (C) 2009-2012  CZ.NIC, z.s.p.o. (http://www.nic.cz)
#*
#*    This library is free software; you can redistribute it and/or
#*    modify it under the terms of the GNU Library General Public
#*    License as published by the Free Software Foundation; either
#*    version 2 of the License, or (at your option) any later version.
#*
#*    This library is distributed in the hope that it will be useful,
#*    but WITHOUT ANY WARRANTY; without even the implied warranty of
#*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#*    Library General Public License for more details.
#*
#*    You should have received a copy of the GNU Library General Public
#*    License along with this library; if not, write to the Free
#*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#*
'''
This module contains methods which can look for a certificate among other 
certificates.
'''

# standard library imports
import logging


def _get_tbs_certificate(certificate):
    '''
    Finds the 'tbsCertificate' component. Its position may
    vary in different forms of used certificates.
    '''
    try:
      return certificate.getComponentByName('tbsCertificate')
    except:
      try:
        return certificate.\
              getComponentByPosition(0).getComponentByName("tbsCertificate")
      except:
        return None
    return None
            
  

def find_cert_by_subject(subject, certs):
    '''
    Looks for the certificate with specified subject.
    '''
    for cert in certs:
        tbs = _get_tbs_certificate(cert)
        subj = str(tbs.getComponentByName("subject"))
        if subj == subject:
            return cert
    return None


def find_cert_by_serial(serial_number, certificates):
    '''
    Looks for certificate with serial_number.
    Returns the certificate or None.
    '''
    for cert in certificates:
        try:
            tbs = _get_tbs_certificate(cert)
            sn = tbs.getComponentByName("serialNumber")                    
            if sn == serial_number:
                return cert
        except Exception, ex:
            logging.error(ex)
            continue
    return None

def find_cert_by_iss_sn(certs, issuer, sn):
  '''
  Looks for certificate with specified issuer and serial number.
  Difference is that certs are objects pkcs7_models.X509certificate,
  not pyAsn components.
  '''
  for cert in certs:
    tbs = _get_tbs_certificate(cert)
    logging.debug("Cert issuer/from DER: %s" % str(tbs.issuer))
    logging.debug("Cert issuer/from python obj: %s" % issuer)      
    if str(tbs.issuer) == issuer:
      if tbs.serial_number == sn:
        return cert
  return None 

def find_cert_in_crl(cert_sn, crl):
    '''
    Looks for certificate with specified serial number in
    certificate revocation list.
    '''
    revoked = crl.getComponentByName("tbsCertList").\
                getComponentByName("revokedCertificates")
    for cert in revoked:
        csn = int(cert.getComponentByName("userCertificate"))
        if csn == cert_sn:
            return csn
    
    return None