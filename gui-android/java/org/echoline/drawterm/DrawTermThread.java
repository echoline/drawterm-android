package org.echoline.drawterm;

/**
 * Created by eli on 12/4/17.
 */

public class DrawTermThread extends Thread {
	private MainActivity m;
	private String c, a, u, p;

	public DrawTermThread(String c, String a, String u, String p, MainActivity m) {
		this.c = c;
		this.a = a;
		this.u = u;
		this.p = p;
		this.m = m;
	}

	@Override
	public void run() {
		String args[] = {"drawterm", "-p", "-h", c, "-a", a, "-u", u};
		m.setPass(p);
		m.dtmain(args);
		m.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				m.setContentView(R.layout.server_main);
				m.populateServers(m);
			}
		});
	}
}
