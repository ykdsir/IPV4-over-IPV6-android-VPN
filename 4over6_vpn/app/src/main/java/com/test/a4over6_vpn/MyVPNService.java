package com.test.a4over6_vpn;

import android.content.Intent;
import android.net.VpnService;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Created by Administrator on 2017/4/26.
 */

public class MyVPNService extends VpnService{
    static final int MAX_BUF = 1024;
    byte[] readBuf = new byte[MAX_BUF];
    String ipv4Addr;// = "13.8.0.2";
    String router;//="0.0.0.0";
    String dns1;//="59.66.16.64";
    String dns2;//="8.8.8.8";
    String dns3;//="202.106.0.20";
    String sockfd;
    Builder builder = new Builder();
    private static ParcelFileDescriptor m_interface;
    public void onCreate()
    {
        Log.d("wjf","VPNService onCreate");
        super.onCreate();

    }

    public void onDestroy()
    {
        super.onDestroy();
        try {
            Log.d("wjf","ondestroy");
            m_interface.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    public int onStartCommand(Intent intent,int flags,int startId)
    {
        try {
            String info = intent.getStringExtra("info");
            if(info.equals("disconnect"))
            {
                closeInterface();
                return START_STICKY;
            }

            ipv4Addr = intent.getStringExtra("ipv4Addr");
            if (ipv4Addr == null) Log.e("wjf", "Ipv4 is null");
            else Log.d("wjf", ipv4Addr);

            router   = intent.getStringExtra("router");
            if (router == null) Log.e("wjf", "Router is null");
            else Log.d("wjf", router);

            dns1     = intent.getStringExtra("dns1");
            Log.d("wjf", dns1);
            dns2     = intent.getStringExtra("dns2");
            if (dns2 == null) Log.e("wjf", "DNS2 is NULL");
            else Log.d("wjf", "DNS2 is " + dns2);


            dns3     = intent.getStringExtra("dns3");
            if (dns3 == null) {
                Log.e("wjf", "DNS3 is NULL");
            } else
            {
                Log.d("wjf", "DNS3 is " + dns3);
            }

            sockfd = intent.getStringExtra("sockfd");

            Log.d("wjf", sockfd);
            builder.setMtu(1460);
            try{
                builder.addAddress(ipv4Addr,32);
            }
            catch (IllegalArgumentException e)
            {
                e.printStackTrace();
            }
            builder.addRoute("0.0.0.0", 0); // router is "0.0.0.0" by default
            builder.addDnsServer(dns1);
            builder.addDnsServer(dns2);
            builder.addDnsServer(dns3);
            builder.setSession("killourselves");//有俩没用到，sockfd??????
            if (protect(Integer.parseInt(sockfd))) {
                Log.d("wjf", "SockFD protected "+ sockfd);
            } else {
                Log.e("wjf", "SockFd not protected " + sockfd);
            }
             m_interface = builder.establish();
            if(m_interface == null)
            {
                Log.e("wjf","m_interface is null");
            }
            Log.d("wjf","before write fd");
            int fd = m_interface.getFd();
            Log.d("fd","fd "+fd);
            writeFD(fd);
        } catch (NullPointerException e) {
            e.printStackTrace();
        }
        Log.e("wjf", "Null pointer exception in Builder");
        return START_STICKY;
    }
    private void writeFD(int fd)
    {
        Log.d("wjf","fd "+fd);
        String extDir = getApplicationInfo().dataDir;
        Log.d("wjf","extdir"+extDir.toString());
        File ipTunnel = new File(extDir,"ip_pipe");
        try{
            Log.d("wjf", "ipTunnel: "+ipTunnel.toString());
            FileOutputStream fileOutputStream = new FileOutputStream(ipTunnel);
            Log.d("wjf", "ipTunnel: "+ipTunnel.toString());
            BufferedOutputStream out = new BufferedOutputStream(fileOutputStream);
            Log.d("wjf", "ipTunnel: "+ipTunnel.toString());
            String fdstr = String.valueOf(fd);
            Log.d("wjf", "fdstr: "+fdstr);
            out.write(fdstr.getBytes());
            out.flush();
            out.close();
            Log.d("wjf","have written down");
            setFD();
        }
        catch (IOException e) {
            e.printStackTrace();
        }

    }
    public static void closeInterface()
    {
        try {
            m_interface.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public native void setFD();
}
