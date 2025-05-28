import React from 'react';

const SidebarLayout = ({ children }: { children: React.ReactNode }) => {
  return (
    <div className="flex h-screen">
      <aside className="w-64 bg-gray-800 text-white p-4">
        <h3 className="text-xl font-bold">Sidebar</h3>
        <nav>
          <ul>
            <li><a href="/dashboard">Dashboard</a></li>
          </ul>
        </nav>
      </aside>
      <main className="flex-1 bg-gray-100 overflow-auto">{children}</main>
    </div>
  );
};

export default SidebarLayout;
