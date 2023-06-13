#include <asm/page.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

struct foo_file
{
    int pid;
    bool print_holes;
};

struct task_struct *find_task_by_pid_nr(int pid)
{
    struct pid *vpid;

    vpid = find_vpid(pid);
    if (!vpid)
        return NULL;

    return pid_task(vpid, PIDTYPE_PID);
}
struct pte_iter
{
    struct seq_file *f;
    unsigned long first_good;
};

static void pte_iter_good(struct pte_iter *it, unsigned long va)
{
    if (it->first_good == 0)
    {
        it->first_good = va;
    }
}
static void pte_iter_end(struct pte_iter *it, unsigned long va)
{
    if (it->first_good == 0)
    {
        return;
    }
    seq_printf(it->f, "%lx-%lx\n", it->first_good, va);
    it->first_good = 0;
}

static int foo_proc_show(struct seq_file *m, void *v)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    unsigned long va;
    struct task_struct *p;
    struct vm_area_struct *vma;

    struct pte_iter it = {
        .f = m,
        .first_good = 0,
    };

    struct foo_file *ff = (struct foo_file *)m->private;
    if (ff->pid == 0)
    {
        p = current;
        seq_printf(m, "pid %s %px\n", "current", p);
    }
    else
    {
        p = find_task_by_pid_nr(ff->pid);
        seq_printf(m, "pid %d %px\n", ff->pid, p);
    }

    if (p == NULL)
    {
        seq_printf(m, "null\n");
    }
    else
    {

        mmap_read_lock(p->mm);

        for (vma = p->mm->mmap; vma; vma = vma->vm_next)
        {

            for (va = vma->vm_start; va < vma->vm_end; va += PAGE_SIZE)
            {
                pgd = pgd_offset(p->mm, va);
                if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
                {

                    pte_iter_end(&it, va);
                    continue;
                }

                p4d = p4d_offset(pgd, va);
                if (p4d_none(*p4d) || unlikely(p4d_bad(*p4d)))
                {
                    pte_iter_end(&it, va);
                    continue;
                }

                pud = pud_offset(p4d, va);
                if (pud_none(*pud) || unlikely(pud_bad(*pud)))
                {
                    pte_iter_end(&it, va);
                    continue;
                }

                pmd = pmd_offset(pud, va);
                if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
                {
                    pte_iter_end(&it, va);
                    continue;
                }

                if (!pmd_present(*pmd))
                {
                    pte_iter_end(&it, va);
                    continue;
                }

                pte = pte_offset_map(pmd, va);
                if (!pte)
                {
                    pte_iter_end(&it, va);
                    continue;
                }

                if (pte_present(*pte))
                {
                    pte_iter_good(&it, va);
                }
                else
                {
                    pte_iter_end(&it, va);
                }

                pte_unmap(pte);
            }
            pte_iter_end(&it, va);
        }
        mmap_read_unlock(p->mm);
    }

    return 0;
}

static int foo_proc_open(struct inode *inode, struct file *file)
{
    struct foo_file *ff = kmalloc(sizeof(struct foo_file), GFP_KERNEL_ACCOUNT);
    ff->pid = 0;
    ff->print_holes = false;
    return single_open(file, foo_proc_show, ff);
}

#define MY_MAGIC 'G'
#define CMD_SET_PID _IOW(MY_MAGIC, 0, int)

static long int foo_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct seq_file *sf;
    struct foo_file *ff;
    sf = ((struct seq_file *)f->private_data);
    ff = sf->private;

    switch (cmd)
    {
    case CMD_SET_PID:
        if (copy_from_user(&ff->pid, (int __user *)arg, sizeof(ff->pid)))
            return -EFAULT;
        pr_info("foo_ioctl pid = %d \n", ff->pid);
        break;

    default:
        return -1;
    }
    return 0;
}

static const struct proc_ops foo_proc_fops = {
    .proc_open = foo_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_ioctl = foo_ioctl,
    .proc_compat_ioctl = foo_ioctl,
    .proc_release = single_release,
};

static int __init foo_init(void)
{

    proc_create("foo", 0, NULL, &foo_proc_fops);
    return 0;
}

static void __exit foo_exit(void)
{
    remove_proc_entry("foo", NULL);
}

module_init(foo_init);
module_exit(foo_exit);