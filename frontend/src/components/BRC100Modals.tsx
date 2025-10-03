import React, { useState } from 'react';
import type { BEEFTransaction, AuthChallengeRequest, BEEFAction } from '../bridge/brc100';

interface DomainApprovalModalProps {
  isOpen: boolean;
  domain: string;
  request: {
    endpoint: string;
    method: string;
    purpose: string;
  };
  onApprove: (whitelist: boolean, oneTime: boolean) => void;
  onReject: () => void;
}

export const DomainApprovalModal: React.FC<DomainApprovalModalProps> = ({
  isOpen,
  domain,
  request,
  onApprove,
  onReject
}) => {
  const [whitelistDomain, setWhitelistDomain] = useState(true); // Checked by default
  const [oneTimeOnly, setOneTimeOnly] = useState(false);

  if (!isOpen) return null;

  const handleApprove = () => {
    onApprove(whitelistDomain, oneTimeOnly);
  };

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg p-6 max-w-md w-full mx-4">
        <div className="flex items-center mb-4">
          <div className="w-8 h-8 bg-orange-500 rounded-full flex items-center justify-center mr-3">
            <svg className="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-2.5L13.732 4c-.77-.833-1.964-.833-2.732 0L3.732 16.5c-.77.833.192 2.5 1.732 2.5z" />
            </svg>
          </div>
          <h2 className="text-xl font-semibold text-gray-900">Domain Approval</h2>
        </div>

        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Requesting Domain
            </label>
            <div className="text-sm text-gray-900 bg-gray-50 p-2 rounded border">
              {domain}
            </div>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Request Type
            </label>
            <div className="text-sm text-gray-900 bg-gray-50 p-2 rounded border">
              {request.method} {request.endpoint}
            </div>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Purpose
            </label>
            <div className="text-sm text-gray-900 bg-gray-50 p-2 rounded border">
              {request.purpose}
            </div>
          </div>

          {/* Approval Options */}
          <div className="space-y-3 p-3 bg-blue-50 rounded border">
            <div className="flex items-center">
              <input
                type="checkbox"
                id="whitelist"
                checked={whitelistDomain}
                onChange={(e) => setWhitelistDomain(e.target.checked)}
                className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
              />
              <label htmlFor="whitelist" className="ml-2 text-sm text-gray-700">
                <span className="font-medium">Add to whitelist</span>
                <span className="text-gray-500"> - Allow future requests from this domain</span>
              </label>
            </div>

            <div className="flex items-center">
              <input
                type="checkbox"
                id="oneTime"
                checked={oneTimeOnly}
                onChange={(e) => {
                  setOneTimeOnly(e.target.checked);
                  if (e.target.checked) {
                    setWhitelistDomain(false); // Can't whitelist if one-time only
                  }
                }}
                className="h-4 w-4 text-blue-600 focus:ring-blue-500 border-gray-300 rounded"
              />
              <label htmlFor="oneTime" className="ml-2 text-sm text-gray-700">
                <span className="font-medium">One-time only</span>
                <span className="text-gray-500"> - Allow this request only</span>
              </label>
            </div>
          </div>

          <div className="bg-orange-50 border border-orange-200 rounded p-3">
            <div className="flex items-start">
              <svg className="w-5 h-5 text-orange-500 mr-2 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
              </svg>
              <div className="text-sm text-orange-800">
                <p className="font-medium">Security Notice:</p>
                <p>Only approve domains you trust. Whitelisted domains can request authentication and transactions from your wallet.</p>
              </div>
            </div>
          </div>
        </div>

        <div className="flex space-x-3 mt-6">
          <button
            onClick={onReject}
            className="flex-1 px-4 py-2 border border-gray-300 rounded-md text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
          >
            Reject
          </button>
          <button
            onClick={handleApprove}
            className="flex-1 px-4 py-2 bg-blue-600 border border-transparent rounded-md text-sm font-medium text-white hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
          >
            Approve
          </button>
        </div>
      </div>
    </div>
  );
};

interface AuthApprovalModalProps {
  isOpen: boolean;
  request: AuthChallengeRequest & { challenge: string };
  onApprove: () => void;
  onReject: () => void;
}

export const AuthApprovalModal: React.FC<AuthApprovalModalProps> = ({
  isOpen,
  request,
  onApprove,
  onReject
}) => {
  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg p-6 max-w-md w-full mx-4">
        <div className="flex items-center mb-4">
          <div className="w-8 h-8 bg-blue-500 rounded-full flex items-center justify-center mr-3">
            <svg className="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v6a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z" />
            </svg>
          </div>
          <h2 className="text-xl font-semibold text-gray-900">Authentication Request</h2>
        </div>

        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Application
            </label>
            <div className="text-sm text-gray-900 bg-gray-50 p-2 rounded border">
              {request.appId}
            </div>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Purpose
            </label>
            <div className="text-sm text-gray-900 bg-gray-50 p-2 rounded border">
              {request.purpose}
            </div>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Challenge
            </label>
            <div className="text-xs text-gray-600 bg-gray-50 p-2 rounded border font-mono break-all">
              {request.challenge}
            </div>
          </div>

          <div className="bg-blue-50 border border-blue-200 rounded p-3">
            <div className="flex items-start">
              <svg className="w-5 h-5 text-blue-500 mr-2 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
              </svg>
              <div className="text-sm text-blue-800">
                <p className="font-medium">What this means:</p>
                <p>The application is requesting permission to authenticate you using BRC-100. This will create a secure session between your wallet and the application.</p>
              </div>
            </div>
          </div>
        </div>

        <div className="flex space-x-3 mt-6">
          <button
            onClick={onReject}
            className="flex-1 px-4 py-2 border border-gray-300 rounded-md text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
          >
            Reject
          </button>
          <button
            onClick={onApprove}
            className="flex-1 px-4 py-2 bg-blue-600 border border-transparent rounded-md text-sm font-medium text-white hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500"
          >
            Approve
          </button>
        </div>
      </div>
    </div>
  );
};

interface TransactionApprovalModalProps {
  isOpen: boolean;
  transaction: BEEFTransaction;
  onApprove: () => void;
  onReject: () => void;
}

export const TransactionApprovalModal: React.FC<TransactionApprovalModalProps> = ({
  isOpen,
  transaction,
  onApprove,
  onReject
}) => {
  if (!isOpen) return null;

  const formatValue = (value: number) => {
    return (value / 100000000).toFixed(8) + ' BSV';
  };

  const getActionIcon = (action: BEEFAction) => {
    switch (action.type) {
      case 'payment':
        return (
          <svg className="w-5 h-5 text-green-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8c-1.657 0-3 .895-3 2s1.343 2 3 2 3 .895 3 2-1.343 2-3 2m0-8c1.11 0 2.08.402 2.599 1M12 8V7m0 1v8m0 0v1m0-1c-1.11 0-2.08-.402-2.599-1" />
          </svg>
        );
      case 'data_transfer':
        return (
          <svg className="w-5 h-5 text-blue-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M9 19l3 3m0 0l3-3m-3 3V10" />
          </svg>
        );
      default:
        return (
          <svg className="w-5 h-5 text-gray-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 12h6m-6 4h6m2 5H7a2 2 0 01-2-2V5a2 2 0 012-2h5.586a1 1 0 01.707.293l5.414 5.414a1 1 0 01.293.707V19a2 2 0 01-2 2z" />
          </svg>
        );
    }
  };

  const getActionDescription = (action: BEEFAction) => {
    switch (action.type) {
      case 'payment':
        return `Send ${formatValue(action.data.amount || 0)} to ${action.data.recipient || 'Unknown'}`;
      case 'data_transfer':
        return `Transfer data: ${action.data.description || 'No description'}`;
      default:
        return `${action.type}: ${JSON.stringify(action.data)}`;
    }
  };

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg p-6 max-w-2xl w-full mx-4 max-h-[90vh] overflow-y-auto">
        <div className="flex items-center mb-4">
          <div className="w-8 h-8 bg-green-500 rounded-full flex items-center justify-center mr-3">
            <svg className="w-5 h-5 text-white" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
            </svg>
          </div>
          <h2 className="text-xl font-semibold text-gray-900">Transaction Approval</h2>
        </div>

        <div className="space-y-4">
          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Application
            </label>
            <div className="text-sm text-gray-900 bg-gray-50 p-2 rounded border">
              {transaction.appDomain}
            </div>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Session ID
            </label>
            <div className="text-xs text-gray-600 bg-gray-50 p-2 rounded border font-mono">
              {transaction.sessionId}
            </div>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Actions ({transaction.actions.length})
            </label>
            <div className="space-y-2">
              {transaction.actions.map((action, index) => (
                <div key={index} className="flex items-start p-3 bg-gray-50 rounded border">
                  <div className="mr-3 mt-0.5">
                    {getActionIcon(action)}
                  </div>
                  <div className="flex-1">
                    <div className="text-sm font-medium text-gray-900">
                      {getActionDescription(action)}
                    </div>
                    <div className="text-xs text-gray-500 mt-1">
                      Type: {action.type} • Time: {new Date(action.timestamp).toLocaleString()}
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>

          {transaction.spvData && (
            <div>
              <label className="block text-sm font-medium text-gray-700 mb-1">
                SPV Data
              </label>
              <div className="text-xs text-gray-600 bg-gray-50 p-2 rounded border">
                <div>Merkle Proofs: {transaction.spvData.merkleProofs.length}</div>
                <div>Block Headers: {transaction.spvData.blockHeaders.length}</div>
                <div>Transaction Data: {transaction.spvData.transactionData.length}</div>
                <div>Identity Proofs: {transaction.spvData.identityProofs.length}</div>
              </div>
            </div>
          )}

          <div className="bg-green-50 border border-green-200 rounded p-3">
            <div className="flex items-start">
              <svg className="w-5 h-5 text-green-500 mr-2 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
              </svg>
              <div className="text-sm text-green-800">
                <p className="font-medium">BEEF Transaction</p>
                <p>This is a BRC-100 BEEF transaction with SPV verification. It will be securely broadcast to the Bitcoin SV network.</p>
              </div>
            </div>
          </div>
        </div>

        <div className="flex space-x-3 mt-6">
          <button
            onClick={onReject}
            className="flex-1 px-4 py-2 border border-gray-300 rounded-md text-sm font-medium text-gray-700 bg-white hover:bg-gray-50 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500"
          >
            Reject
          </button>
          <button
            onClick={onApprove}
            className="flex-1 px-4 py-2 bg-green-600 border border-transparent rounded-md text-sm font-medium text-white hover:bg-green-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-green-500"
          >
            Approve & Broadcast
          </button>
        </div>
      </div>
    </div>
  );
};

// Modal Manager Hook
export const useBRC100Modals = () => {
  const [authModal, setAuthModal] = useState<{
    isOpen: boolean;
    request: AuthChallengeRequest & { challenge: string } | null;
    resolve: ((value: boolean) => void) | null;
  }>({
    isOpen: false,
    request: null,
    resolve: null
  });

  const [transactionModal, setTransactionModal] = useState<{
    isOpen: boolean;
    transaction: BEEFTransaction | null;
    resolve: ((value: boolean) => void) | null;
  }>({
    isOpen: false,
    transaction: null,
    resolve: null
  });

  const [domainModal, setDomainModal] = useState<{
    isOpen: boolean;
    domain: string;
    request: any;
    resolve: ((value: { whitelist: boolean; oneTime: boolean }) => void) | null;
  }>({
    isOpen: false,
    domain: '',
    request: null,
    resolve: null
  });

  const showAuthApprovalModal = (
    request: AuthChallengeRequest & { challenge: string }
  ): Promise<boolean> => {
    return new Promise((resolve) => {
      setAuthModal({
        isOpen: true,
        request,
        resolve
      });
    });
  };

  const showTransactionApprovalModal = (transaction: BEEFTransaction): Promise<boolean> => {
    return new Promise((resolve) => {
      setTransactionModal({
        isOpen: true,
        transaction,
        resolve
      });
    });
  };

  const showDomainApprovalModal = (
    domain: string,
    request: { method: string; endpoint: string; purpose: string }
  ): Promise<{ whitelist: boolean; oneTime: boolean }> => {
    return new Promise((resolve) => {
      setDomainModal({
        isOpen: true,
        domain,
        request,
        resolve
      });
    });
  };

  const handleAuthApprove = () => {
    if (authModal.resolve) {
      authModal.resolve(true);
    }
    setAuthModal({ isOpen: false, request: null, resolve: null });
  };

  const handleAuthReject = () => {
    if (authModal.resolve) {
      authModal.resolve(false);
    }
    setAuthModal({ isOpen: false, request: null, resolve: null });
  };

  const handleTransactionApprove = () => {
    if (transactionModal.resolve) {
      transactionModal.resolve(true);
    }
    setTransactionModal({ isOpen: false, transaction: null, resolve: null });
  };

  const handleTransactionReject = () => {
    if (transactionModal.resolve) {
      transactionModal.resolve(false);
    }
    setTransactionModal({ isOpen: false, transaction: null, resolve: null });
  };

  const handleDomainApprove = (whitelist: boolean, oneTime: boolean) => {
    if (domainModal.resolve) {
      domainModal.resolve({ whitelist, oneTime });
    }
    setDomainModal({ isOpen: false, domain: '', request: null, resolve: null });
  };

  const handleDomainReject = () => {
    if (domainModal.resolve) {
      domainModal.resolve({ whitelist: false, oneTime: false });
    }
    setDomainModal({ isOpen: false, domain: '', request: null, resolve: null });
  };

  return {
    authModal,
    transactionModal,
    domainModal,
    showAuthApprovalModal,
    showTransactionApprovalModal,
    showDomainApprovalModal,
    handleAuthApprove,
    handleAuthReject,
    handleTransactionApprove,
    handleTransactionReject,
    handleDomainApprove,
    handleDomainReject
  };
};
